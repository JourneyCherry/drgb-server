#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "MyInvoker.hpp"
#include "MyMsg.hpp"
#include "Thread.hpp"
#include "MyExcepts.hpp"

class MyConnector
{
	protected: // Child Class가 세팅해야 되는 변수들.
		std::string target_addr;
		int target_port;
	
	private:
		using Thread = mylib::threads::Thread;

		bool isRunning;
		MyCommon::Invoker<bool> isConnected;

		int socket_fd;
		struct sockaddr_in addr;
		std::mutex m_req;
		const int BUFSIZE = 1024;
		const int RETRY_WAIT_SEC = 3;
	
		Thread t_conn;
		Thread t_recv;
		MyMsg recvbuffer;

	protected:
		std::string keyword;

	private:
		void Connect_(std::shared_ptr<bool>);
		void RecvLoop(std::shared_ptr<bool>);

	public:
		MyConnector(MyThreadExceptInterface*, std::string, int, std::string);
		virtual ~MyConnector();
		void Connect();
		void Disconnect();
		MyBytes Request(MyBytes);
};