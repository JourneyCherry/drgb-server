#pragma once
#include <mutex>
#include <thread>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include "MyServerSocket.hpp"
#include "MyTCPClient.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::Expected;

class MyTCPServer : public MyServerSocket
{
	private:
		static constexpr int LISTEN_SIZE = 5;

		int server_fd;
	public:
		MyTCPServer(int);
		MyTCPServer(const MyTCPServer&) = delete;
		MyTCPServer(MyTCPServer&&) = delete;
		~MyTCPServer();

		Expected<std::shared_ptr<MyClientSocket>, int> Accept() override;
		void Close() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};