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

MyExpected<MyBytes, int> MyTCPClient::Recv()
{
	unsigned char buf[BUFSIZE];
	while(!recvbuffer.isMsgIn())
	{
		if(socket_fd < 0)
			return {-1};
		int recvlen = read(socket_fd, buf, BUFSIZE);
		if(recvlen <= 0)
			return {recvlen == 0?0:errno};

		recvbuffer.Recv(buf, recvlen);
	}

	return recvbuffer.GetMsg();
}

MyExpected<int> MyTCPClient::Send(MyBytes bytes)
{
	if(socket_fd < 0)
		return {-1, false};
	MyBytes capsulated = MyMsg::enpackage(bytes);
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