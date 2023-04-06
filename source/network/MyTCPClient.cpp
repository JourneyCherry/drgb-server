#include "MyTCPClient.hpp"

MyTCPClient::MyTCPClient(int fd, std::string addr) : MyClientSocket()
{
	socket_fd = fd;
	Address = addr;
}

MyTCPClient::~MyTCPClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyTCPClient::RecvRaw()
{
	unsigned char buf[BUFSIZE];
	std::vector<byte> result_bytes;

	if(socket_fd < 0)
		return {ErrorCode{ERR_CONNECTION_CLOSED}};
	int recvlen = read(socket_fd, buf, BUFSIZE);
	if(recvlen < 0)
		return {ErrorCode{errno}};
	if(recvlen == 0)
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	result_bytes.assign(buf, buf + recvlen);

	return result_bytes;
}

ErrorCode MyTCPClient::SendRaw(const byte* bytes, const size_t &len)
{
	if(socket_fd < 0)
		return {ERR_CONNECTION_CLOSED};
	int sendlen = send(socket_fd, bytes, len, 0);
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};
	if(sendlen < 0)
		return {errno};

	return {SUCCESS};
}

StackErrorCode MyTCPClient::Connect(std::string addr, int port)
{
	Close();	//이미 열려있는 소켓은 닫기.

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0)
		return {errno, __STACKINFO__};

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, addr.c_str(), &(server_addr.sin_addr.s_addr)) <= 0)
	{
		struct addrinfo hints;
		struct addrinfo *serverinfo;

		char bport[6];

		memset(bport, 0, sizeof(port));
		sprintf(bport, "%hu", port);

		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		int result_getaddr = getaddrinfo(addr.c_str(), bport, &hints, &serverinfo);
		if (result_getaddr != 0)
			return {result_getaddr, __STACKINFO__};
		memcpy(&server_addr, serverinfo->ai_addr, sizeof(struct sockaddr));

		freeaddrinfo(serverinfo);
	}

	if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		return {errno, __STACKINFO__};

	Address = std::string(inet_ntoa(server_addr.sin_addr)) + ":" + std::to_string(server_addr.sin_port);
	
	return {};
}

void MyTCPClient::Close()
{
	if(socket_fd >= 0)
	{
		shutdown(socket_fd, SHUT_RDWR);
		socket_fd = -1;
	}
}