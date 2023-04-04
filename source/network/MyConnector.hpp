#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "Invoker.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "Thread.hpp"
#include "Logger.hpp"

using mylib::utils::ByteQueue;
using mylib::utils::StackTraceExcept;
using mylib::utils::PacketProcessor;
using mylib::utils::Logger;
using mylib::utils::Invoker;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;

class MyConnector
{
	protected: // Child Class가 세팅해야 되는 변수들.
		std::string target_addr;
		int target_port;
	
	private:

		bool isRunning;
		Invoker<bool> isConnected;

		int socket_fd;
		struct sockaddr_in addr;
		std::mutex m_req;
		const int BUFSIZE = 1024;
		const int RETRY_WAIT_SEC = 3;
	
		Thread t_conn;
		Thread t_recv;
		PacketProcessor recvbuffer;

	protected:
		std::string keyword;

	private:
		void Connect_();
		void RecvLoop();

	public:
		MyConnector(ThreadExceptHandler*, std::string, int, std::string);
		virtual ~MyConnector();
		void Connect();
		void Disconnect();
		ByteQueue Request(ByteQueue);
};