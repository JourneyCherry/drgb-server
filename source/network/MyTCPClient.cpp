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

Expected<ByteQueue, int> MyTCPClient::Recv()
{
	unsigned char buf[BUFSIZE];
	while(!recvbuffer.isMsgIn())
	{
		if(socket_fd < 0)
			return {-1};
		int recvlen = read(socket_fd, buf, BUFSIZE);
		if(recvlen < 0)
			return {errno};
		if(recvlen == 0)
			return {-1};

		recvbuffer.Recv(buf, recvlen);
	}

	return recvbuffer.GetMsg();
}

Expected<int> MyTCPClient::Send(ByteQueue bytes)
{
	if(socket_fd < 0)
		return {-1, false};
	ByteQueue capsulated = PacketProcessor::enpackage(bytes);
	int sendlen = send(socket_fd, capsulated.data(), capsulated.Size(), 0);
	if(sendlen <= 0)
		return {sendlen == 0?0:errno, false};

	return {0, true};
}

void MyTCPClient::Close()
{
	if(socket_fd >= 0)
	{
		shutdown(socket_fd, SHUT_RDWR);
		socket_fd = -1;
	}
}