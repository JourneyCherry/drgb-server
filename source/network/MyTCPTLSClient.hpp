#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
//For SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
////
#include "MyClientSocket.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyTCPTLSClient : public MyClientSocket
{
	private:
		static constexpr int BUFSIZE = 1024;
		SSL *ssl;
		int socket_fd;
	public:
		MyTCPTLSClient() : socket_fd(-1), MyClientSocket() {}
		MyTCPTLSClient(int, SSL*, std::string);	//TCPServer에서 Accept한 socket으로 생성할 때의 생성자.
		MyTCPTLSClient(const MyTCPTLSClient&) = delete;
		MyTCPTLSClient(MyTCPTLSClient&&) = delete;
		~MyTCPTLSClient();
	public:
		StackErrorCode Connect(std::string, int) override;
		void Close() override;

		MyTCPTLSClient& operator=(const MyTCPTLSClient&) = delete;
		MyTCPTLSClient& operator=(MyTCPTLSClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
		ErrorCode GetSSLErrorFromRet(int);
};