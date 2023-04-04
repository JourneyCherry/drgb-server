#include "MyConnectee.hpp"

MyConnectee::MyConnectee(ThreadExceptHandler *parent)
 : t_accept(std::bind(&MyConnectee::AcceptLoop, this), this, false), ClientPool(this), ThreadExceptHandler(parent)
{
	server_fd = -1;
	port = -1;
	isRunning = false;
	opt_reuseaddr = 1;
}

MyConnectee::~MyConnectee()
{
	Close();
}

void MyConnectee::Open(int p)
{
	port = p;
	if (server_fd >= 0)
		return;

	isRunning = true;
	t_accept.start();
}

void MyConnectee::Close()
{
	isRunning = false;
	if (server_fd >= 0)
		shutdown(server_fd, SHUT_RDWR);

	t_accept.join();
	ClientPool.safe_loop([&](int fd){ shutdown(fd, SHUT_RDWR); });
	ClientPool.Stop();
}

void MyConnectee::Accept(std::string keyword, std::function<ByteQueue(ByteQueue)> process)
{
	if(!server_fd.wait([](int fd){return fd >= 0;}))
		throw StackTraceExcept("MyConnectee::Accept : server_fd is not opened", __STACKINFO__);
	
	KeywordProcessMap.insert_or_assign(keyword, process);
}

void MyConnectee::AcceptLoop()
{
	Thread::SetThreadName("ConnectAcceptor");

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw StackTraceExcept("MyConnectee::Open()::socket() Failed : " + std::to_string(errno), __STACKINFO__);
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const int*)&opt_reuseaddr, sizeof(opt_reuseaddr)) < 0)
		throw StackTraceExcept("MyConnectee::Open()::setsockopt() Failed : " + std::to_string(errno), __STACKINFO__);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		throw StackTraceExcept("MyConnectee::Open()::bind() Failed : " + std::to_string(errno), __STACKINFO__);
	if(listen(fd, LISTENSIZE) < 0)
		throw StackTraceExcept("MyConnectee::Open()::listen() Failed : " + std::to_string(errno), __STACKINFO__);

	if(server_fd > 0)
		shutdown(server_fd, SHUT_RDWR);
	server_fd = fd;

	while(isRunning)
	{
		ClientPool.flush();	//주기적으로(여기선 새 Connector가 들어왔을 때) flush 해줌.
			
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client_fd < 0)
		{
			int error_code = errno;
			if(error_code == EINVAL)	//shutdown(socket_fd, SHUT_RDWR)로부터 반환됨.
				break;
			else
				throw StackTraceExcept("MyConnectee::AcceptLoop::accept : " + std::to_string(error_code), __STACKINFO__);
		}
		char authentication_buffer[BUFSIZE];
		int recvsize = recv(client_fd, authentication_buffer, BUFSIZE, 0);	//TODO : Fail within n seconds.
		if(recvsize <= 0)
		{
			int error_code = errno;
			if(error_code != EINVAL && recvsize < 0)
				Logger::log("Connectee::Accept::recv : " + ErrorCode(error_code).message_code(), Logger::LogType::error);
			shutdown(client_fd, SHUT_RDWR);
			continue;
		}
		//인증용 문자열(즉, 발송지 이름)을 올바르게 받지 못하면 연결을 끊는다.
		std::string authenticate = std::string(authentication_buffer, recvsize);
		if(KeywordProcessMap.find(authenticate) == KeywordProcessMap.end())
		{
			Logger::log("Host <- " + authenticate + " Failed(Unknown keyword)");
			shutdown(client_fd, SHUT_RDWR);
			continue;
		}
		ClientPool.insert(client_fd, std::bind(&MyConnectee::ClientLoop, this, authenticate, client_fd));
	}
	
	shutdown(server_fd, SHUT_RDWR);
	server_fd = -1;
}

void MyConnectee::ClientLoop(std::string keyword, int fd)
{
	Thread::SetThreadName("ConnecteeClientLoop_" + keyword);

	Logger::log("Host <- " + keyword + " is Connected");	//TODO : Remote Address도 로깅 필요
	PacketProcessor recvbuffer;
	while(isRunning)
	{		
		char buffer[BUFSIZE];
		memset(buffer, 0, BUFSIZE);
		int recvlen = recv(fd, buffer, BUFSIZE, 0);
		if(recvlen == 0)
		{
			break;
		}
		else if(recvlen < 0)
		{
			int ec = errno;
			Logger::log("Host <- " + keyword + " is Disconnected");
			if(ec != EINVAL)
				throw ErrorCodeExcept(ec, __STACKINFO__);
			break;
		}
		recvbuffer.Recv((byte*)buffer, recvlen);

		if(!recvbuffer.isMsgIn())
			continue;

		ByteQueue answer;
		try
		{
			ByteQueue data = recvbuffer.GetMsg();
			answer = KeywordProcessMap[keyword](data);
		}
		catch(const std::exception &e)
		{
			answer.Clear();
			answer = ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
		}
		std::vector<byte> msg = PacketProcessor::encapsulate(answer.vector());
		int sendlen = send(fd, msg.data(), msg.size(), 0);
		if(sendlen <= 0)
			throw ErrorCodeExcept(errno, __STACKINFO__);
	}

	Logger::log("Host <- " + keyword + " is Disconnected");
	shutdown(fd, SHUT_RDWR);
}