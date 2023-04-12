#pragma once
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <cerrno>
//For TCP
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
//For SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
////
#include "MyServerSocket.hpp"
#include "MyTCPTLSClient.hpp"
#include "ConfigParser.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::Expected;
using mylib::utils::ConfigParser;

class MyTCPTLSServer : public MyServerSocket
{
	private:
		static constexpr int LISTEN_SIZE = 5;
		SSL_CTX *ctx;

		int server_fd;
	public:
		MyTCPTLSServer(int);
		MyTCPTLSServer(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer(MyTCPTLSServer&&) = delete;
		~MyTCPTLSServer();

		Expected<std::shared_ptr<MyClientSocket>, ErrorCode> Accept() override;
		void Close() override;

		MyTCPTLSServer& operator=(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer& operator=(MyTCPTLSServer&&) = delete;
};