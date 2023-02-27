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
#include "MyExcepts.hpp"

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

		MyExpected<std::shared_ptr<MyClientSocket>, int> Accept() override;
		void Close() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};