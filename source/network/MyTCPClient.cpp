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

void MyTCPClient::Close()
{
	if(socket_fd >= 0)
	{
		shutdown(socket_fd, SHUT_RDWR);
		socket_fd = -1;
	}
}