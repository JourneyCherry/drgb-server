#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include "MyClientSocket.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyTCPClient : public MyClientSocket
{
	private:
		static constexpr int BUFSIZE = 1024;
		int socket_fd;
	public:
		MyTCPClient() : socket_fd(-1), MyClientSocket() {}
		MyTCPClient(int, std::string);	//TCPServer에서 Accept한 socket으로 생성할 때의 생성자.
		MyTCPClient(const MyTCPClient&) = delete;
		MyTCPClient(MyTCPClient&&) = delete;
		~MyTCPClient();
	public:
		ErrorCode Connect(std::string, int) override;
		void Close() override;

		MyTCPClient& operator=(const MyTCPClient&) = delete;
		MyTCPClient& operator=(MyTCPClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
};