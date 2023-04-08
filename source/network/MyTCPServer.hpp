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
#include "MyTCPClient.hpp"
#include "ConfigParser.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::Expected;
using mylib::utils::ConfigParser;

class MyTCPServer : public MyServerSocket
{
	private:
		static constexpr int LISTEN_SIZE = 5;
		SSL_CTX *ctx;

		int server_fd;
	public:
		MyTCPServer(int);
		MyTCPServer(const MyTCPServer&) = delete;
		MyTCPServer(MyTCPServer&&) = delete;
		~MyTCPServer();

		Expected<std::shared_ptr<MyClientSocket>, ErrorCode> Accept() override;
		void Close() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};