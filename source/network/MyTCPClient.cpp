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

Expected<ByteQueue, ErrorCode> MyTCPClient::Recv()
{
	unsigned char buf[BUFSIZE];
	while(!recvbuffer.isMsgIn())
	{
		if(socket_fd < 0)
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		int recvlen = read(socket_fd, buf, BUFSIZE);
		if(recvlen < 0)
			return {ErrorCode{errno}};
		if(recvlen == 0)
			return {ErrorCode{ERR_CONNECTION_CLOSED}};

		recvbuffer.Recv(buf, recvlen);
	}

	return recvbuffer.GetMsg();
}

ErrorCode MyTCPClient::Send(ByteQueue bytes)
{
	if(socket_fd < 0)
		return {ERR_CONNECTION_CLOSED};
	ByteQueue capsulated = PacketProcessor::enpackage(bytes);
	int sendlen = send(socket_fd, capsulated.data(), capsulated.Size(), 0);
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};
	if(sendlen < 0)
		return {errno};

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