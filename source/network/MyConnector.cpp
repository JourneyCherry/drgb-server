#include "MyConnector.hpp"

MyConnector::MyConnector(ThreadExceptHandler* parent, std::string addr, int port, std::string key)
	: t_conn(std::bind(&MyConnector::Connect_, this, std::placeholders::_1), parent, false),
	t_recv(std::bind(&MyConnector::RecvLoop, this, std::placeholders::_1), parent, false)
{
	isRunning = false;
	isConnected = false;
	socket_fd = -1;
	target_addr = addr;
	target_port = port;
	keyword = key;
}

MyConnector::~MyConnector()
{
	Disconnect();
}

void MyConnector::Connect()
{
	if(!isRunning)
	{
		if(keyword.length() <= 0)
			throw StackTraceExcept("MyConnector::Create_Socket() : keyword is null", __STACKINFO__);
		isRunning = true;
		isConnected.use();
		t_conn.start();
		t_recv.start();
	}
}

void MyConnector::Connect_(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "Connector");
#endif
	while(isRunning)
	{
		if(!isConnected.wait(false))
			break;
		
		if(*killswitch)
			return;

		if(socket_fd >= 0)	//소켓이 열려있다면 닫고 시작한다.
			shutdown(socket_fd, SHUT_RDWR);

		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(socket_fd < 0)	//socket file descriptor는 non-negative integer이다. 즉, 0을 포함한 값이 정상값이다.
			throw StackTraceExcept("MyConnector(" + keyword + ") : socket() : " + std::to_string(errno), __STACKINFO__);	//이 예외를 받을 loop가 필요하다.

		char port[6];

		addr.sin_family = AF_INET;
		addr.sin_port = htons(target_port);

		if (inet_pton(AF_INET, target_addr.c_str(), &addr.sin_addr) <= 0)
		{
			struct addrinfo hints;
			struct addrinfo *serverinfo;

			memset(port, 0, sizeof(port));
			sprintf(port, "%hu", target_port);

			memset(&hints, 0, sizeof(hints));
			hints.ai_socktype = SOCK_STREAM;
			int result_getaddr = getaddrinfo(target_addr.c_str(), port, &hints, &serverinfo);
			if (result_getaddr != 0)
			{
				//freeaddrinfo(serverinfo);
				Logger::log("MyConnector::Connect() : Domain name resolution failed. Errno : " + std::to_string(result_getaddr) + " re-search after " + std::to_string(RETRY_WAIT_SEC) + " secs.", Logger::LogType::debug);
				std::this_thread::sleep_for(std::chrono::seconds(RETRY_WAIT_SEC));
				continue;
				//throw StackTraceExcept("Domain name resolution failed");
			}
			memcpy(&addr, serverinfo->ai_addr, sizeof(struct sockaddr));

			freeaddrinfo(serverinfo);
		}
		if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) >= 0)
		{
			int sendsize = send(socket_fd, keyword.c_str(), keyword.size(), 0);
			if (sendsize > 0)
			{
				Logger::log("MyConnector::Connect(" + target_addr + ") : success as " + keyword, Logger::LogType::debug);
				Logger::log("Host -> " + target_addr + " is Connected");
				isConnected = true;
				continue;
			}
			Logger::log("MyConnector::Connect()::send() Failed as " + Logger::strerrno(errno), Logger::LogType::debug);
		}
		else
		{
			switch(errno)
			{
				case ECONNREFUSED:	//연결 거부될 경우.
					Logger::log(keyword + " to " + target_addr + " is refused", Logger::LogType::debug);
					break;
				case ETIMEDOUT:		//연결 시간초과
					Logger::log(keyword + " to " + target_addr + " is Timed Out", Logger::LogType::debug);
					break;
				case EISCONN:		//이미 연결이 된 경우
					Logger::log(keyword + " to " + target_addr + " is already Connected", Logger::LogType::debug);
					isConnected = true;
					continue;
				default:
					Logger::log(keyword + " to " + target_addr + " is Failed by " + Logger::strerrno(errno), Logger::LogType::debug);
					break;
					//TODO : 여기서 throw exception하면 어디서 받아야될지 판단해야 한다.
					//throw MyExternalErrorCode("MyConnector::Connect::connect()", errno);
			}
		}
		if(isRunning)
		{
			Logger::log("MyConnector::Connect() : failed. retry after " + std::to_string(RETRY_WAIT_SEC) + " sec", Logger::LogType::debug);
			std::this_thread::sleep_for(std::chrono::seconds(RETRY_WAIT_SEC));
		}
	}
}

void MyConnector::Disconnect()
{
	if(isRunning)
	{
		isRunning = false;
		isConnected.release();
		shutdown(socket_fd, SHUT_RDWR);
	}
	t_conn.stop();
	t_recv.stop();
	//TODO : t_conn과 t_recv의 exception 처리하기. 단, rethrow를 하면 소멸자에서 예외를 던질 수 있으므로, Logging을 통해서 처리하자.
}

void MyConnector::RecvLoop(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "ConnectorReciver");
#endif
	while(isRunning)
	{
		if(!isConnected.wait(true))
			return;
		
		if(*killswitch)
			return;
		
		recvbuffer.Start();

		char buff[BUFSIZE];
		int readsize = read(socket_fd, buff, BUFSIZE);
		if(readsize == 0)
		{
			recvbuffer.Stop();
			isConnected = false;
			Logger::log("Host -> " + target_addr + " is Disconnected");
			continue;
		}
		else if(readsize < 0)
		{
			Logger::log("MyConnector::RecvLoop()::read() : Get error with \"" + Logger::strerrno(errno) + "\"", Logger::LogType::error);
			recvbuffer.Stop();
			isConnected = false;
			continue;
		}
		recvbuffer.Recv((byte*)buff, readsize);
	}
}

ByteQueue MyConnector::Request(ByteQueue data)
{
	if (data.Size() <= 0)
		throw StackTraceExcept("Request data is empty", __STACKINFO__);

	std::vector<byte> msg = PacketProcessor::enpackage(data);

	while (isRunning)
	{
		if(!isConnected.wait(true))
			throw StackTraceExcept("MyConnector is not working", __STACKINFO__);
		std::unique_lock<std::mutex> lk(m_req);
		int sendsize = send(socket_fd, (const char *)msg.data(), msg.size(), 0);
		//TODO : send 부분 까지만 락을 걸지, 주고/받기 전 과정을 락을 걸지 판단 필요.
		if(sendsize == 0)
			continue;
		else if (sendsize < 0)
			throw StackTraceExcept("MyConnector::Request()::send() : " + std::to_string(errno), __STACKINFO__);	//TODO : 소켓을 상대가 끊었을 때 여기로 오면 예외를 던지는게 맞나?

		return recvbuffer.JoinMsg();	//recvbuffer.isMsgIn()이 true일 때 까지 대기.
	}

	throw StackTraceExcept("MyConnector is not working", __STACKINFO__);
}