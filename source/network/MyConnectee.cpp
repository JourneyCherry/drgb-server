#include "MyConnectee.hpp"

MyConnectee::MyConnectee(MyThreadExceptInterface *parent)
 : t_accept(std::bind(&MyConnectee::AcceptLoop, this, std::placeholders::_1), parent, false)
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
	{
		shutdown(server_fd, SHUT_RDWR);
		server_fd = -1;
	}
	for(auto &[key, pair] : clients)
	{
		shutdown(*pair.second, SHUT_RDWR);
		(*pair.second) = -1;
		pair.second->release();
		pair.first->stop();	//TODO : pair.first.exception 처리
	}
	clients.clear();
	t_accept.stop();//TODO : t_accept.exception 처리
}

void MyConnectee::Accept(std::string keyword, std::function<MyBytes(MyBytes)> process)
{
	if(!server_fd.wait([](int fd){return fd >= 0;}))
		throw MyExcepts("MyConnectee::Accept : server_fd is not opened", __STACKINFO__);
	std::shared_ptr<MyCommon::Invoker<int>> sfd = std::make_shared<MyCommon::Invoker<int>>(-1);
	std::pair<std::shared_ptr<Thread>, std::shared_ptr<MyCommon::Invoker<int>>> p = {std::make_shared<Thread>(std::bind(&MyConnectee::ClientLoop, this, keyword, sfd, process, std::placeholders::_1)), sfd};
	clients.insert({keyword, std::move(p)});
}

void MyConnectee::AcceptLoop(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "ConnectAcceptor");
#endif
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw MyExcepts("MyConnectee::Open()::socket() Failed : " + std::to_string(errno), __STACKINFO__);
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const int*)&opt_reuseaddr, sizeof(opt_reuseaddr)) < 0)
		throw MyExcepts("MyConnectee::Open()::setsockopt() Failed : " + std::to_string(errno), __STACKINFO__);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		throw MyExcepts("MyConnectee::Open()::bind() Failed : " + std::to_string(errno), __STACKINFO__);
	if(listen(fd, LISTENSIZE) < 0)
		throw MyExcepts("MyConnectee::Open()::listen() Failed : " + std::to_string(errno), __STACKINFO__);

	if(server_fd > 0)
		shutdown(server_fd, SHUT_RDWR);
	server_fd = fd;

	while(isRunning)
	{
		if(*killswitch)
			break;
			
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client_fd < 0)
		{
			int error_code = errno;
			if(error_code == EINVAL)	//shutdown(socket_fd, SHUT_RDWR)로부터 반환됨.
				break;
			else
				throw MyExcepts("MyConnectee::AcceptLoop::accept : " + std::to_string(error_code), __STACKINFO__);
		}
		char authentication_buffer[BUFSIZE];
		int recvsize = recv(client_fd, authentication_buffer, BUFSIZE, 0);	//TODO : Fail within n seconds.
		if(recvsize <= 0)
		{
			//TODO : Log error.(whether client close connection or socket causes error)
			shutdown(client_fd, SHUT_RDWR);
			continue;
		}
		//인증용 문자열(즉, 발송지 이름)을 올바르게 받지 못하면 연결을 끊는다.
		std::string authenticate = std::string(authentication_buffer, recvsize);
		if(clients.find(authenticate) == clients.end())
		{
			MyLogger::log("Host <- " + authenticate + " Failed(Unknown keyword)");
			shutdown(client_fd, SHUT_RDWR);
			continue;
		}
		if((*clients[authenticate].second) >= 0)
		{
			//TODO : kill late client? or kick earlier client?
			MyLogger::log("MyConnectee::AcceptLoop(" + authenticate + ") : there is already connected connector.", MyLogger::LogType::debug);
		}
		MyLogger::log("MyConnectee::AcceptLoop(" + authenticate + ") is connected", MyLogger::LogType::debug);
		MyLogger::log("Host <- " + authenticate + " is Connected");
		(*clients[authenticate].second) = client_fd;
	}
	
	shutdown(server_fd, SHUT_RDWR);
	server_fd = -1;
}

void MyConnectee::ClientLoop(
	std::string keyword, 
	std::shared_ptr<MyCommon::Invoker<int>> fd, 
	std::function<MyBytes(MyBytes)> process,
	std::shared_ptr<bool> killswitch
)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), (std::string("ConnecteeLoop_") + keyword).c_str());
#endif
	MyMsg recvbuffer;
	MyLogger::log("MyConnectee::AcceptLoop(" + keyword + ") waits for connect", MyLogger::LogType::debug);
	while(isRunning)
	{
		if(!fd->wait([](int value){return value >= 0;}))
			return;
		
		if(*killswitch)
			return;
		
		char buffer[BUFSIZE];
		memset(buffer, 0, BUFSIZE);
		int recvlen = recv(*fd, buffer, BUFSIZE, 0);
		if(recvlen == 0)
		{
			MyLogger::log("Host <- " + keyword + " is Disconnected");
			shutdown(*fd, SHUT_RDWR);
			*fd = -1;
			recvbuffer.Clear();
			continue;
		}
		else if(recvlen < 0)
		{
			MyLogger::log("Host <- " + keyword + " is Disconnected");
			shutdown(*fd, SHUT_RDWR);
			MyLogger::log("MyConnectee::ClientLoop(" + keyword + ")::recv() : " + MyLogger::strerrno(errno), MyLogger::LogType::debug);
			*fd = -1;
			recvbuffer.Clear();
			continue;
		}
		recvbuffer.Recv((byte*)buffer, recvlen);

		if(!recvbuffer.isMsgIn())
			continue;

		MyBytes answer;
		try
		{
			MyBytes data = recvbuffer.GetMsg();
			answer = process(data);
		}
		catch(const std::exception &e)
		{
			answer.Clear();
			answer = MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION);
		}
		std::vector<byte> msg = MyMsg::enpackage(answer);
		int sendlen = send(*fd, msg.data(), msg.size(), 0);
		if(sendlen <= 0)
			throw MyExcepts("Cannot Send Packet to Client : " + std::to_string(errno), __STACKINFO__);
	}
}