#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include "MyClientSocket.hpp"
#include "MyExpected.hpp"
#include "MyLogger.hpp"

class MyTCPClient : public MyClientSocket
{
	private:
		static constexpr int BUFSIZE = 1024;
		int socket_fd;
	public:
		MyTCPClient(int, std::string);
		MyTCPClient(const MyTCPClient&) = delete;
		MyTCPClient(MyTCPClient&&) = delete;
		~MyTCPClient();
	public:
		MyExpected<MyBytes, int> Recv() override;
		MyExpected<int> Send(MyBytes) override;
		void Close() override;

		MyTCPClient& operator=(const MyTCPClient&) = delete;
		MyTCPClient& operator=(MyTCPClient&&) = delete;
};