#include "MyTCPServer.hpp"

MyTCPServer::MyTCPServer(int p) : MyServerSocket(p)
{
	int opt_reuseaddr = 1;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
		throw MyExcepts("MyTCPServer::Open()::socket() Failed (" + std::to_string(errno) + ")", __STACKINFO__);
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const int*)&opt_reuseaddr, sizeof(opt_reuseaddr)) < 0)
		throw MyExcepts("MyTCPServer::Open()::setsockopt() Failed (" + std::to_string(errno) + ")", __STACKINFO__);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		throw MyExcepts("MyTCPServer::Open()::bind() Failed (" + std::to_string(errno) + ")", __STACKINFO__);
	if(listen(server_fd, LISTEN_SIZE) < 0)
		throw MyExcepts("MyTCPServer::Open()::listen() Failed (" + std::to_string(errno) + ")", __STACKINFO__);
}

MyTCPServer::~MyTCPServer()
{
	Close();
}

void MyTCPServer::Close()
{
	if(server_fd >= 0)
	{
		shutdown(server_fd, SHUT_RDWR);
		server_fd = -1;
	}
}

MyExpected<std::shared_ptr<MyClientSocket>, int> MyTCPServer::Accept()
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof((struct sockaddr *)&client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	if(client_fd < 0)
	{
		int error_code = errno;
		switch(error_code)
		{
			case EINVAL:	//shutdown(socket_fd, SHUT_RDWR)로부터 반환됨.
				return {-1};
		}
		return {error_code};
	}
	return {std::make_shared<MyTCPClient>(client_fd, std::string(inet_ntoa(client_addr.sin_addr)) + std::to_string(client_addr.sin_port))};
}