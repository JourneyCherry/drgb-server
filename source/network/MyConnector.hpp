#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "MyTCPClient.hpp"
#include "MyWebsocketClient.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "Thread.hpp"
#include "Logger.hpp"

using mylib::utils::ByteQueue;
using mylib::utils::StackErrorCode;
using mylib::utils::StackTraceExcept;
using mylib::utils::PacketProcessor;
using mylib::utils::Logger;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;

class MyConnector
{
	protected: // Child Class가 세팅해야 되는 변수들.
		std::string target_addr;
		int target_port;
	
	private:
		static constexpr float TIME_WAIT_ANSWER = 1.0f;
		bool isConnecting;
		bool isRunning;
		//MyTCPClient client_socket;
		MyTCPClient socket;
		std::mutex m_req;
		const int RETRY_WAIT_SEC = 3;
	
		Thread t_conn;

	protected:
		std::string keyword;

	private:
		std::condition_variable cv;
		void ConnectLoop();

	public:
		MyConnector(ThreadExceptHandler*, std::string, int, std::string);
		virtual ~MyConnector();
		void Connect();
		void Disconnect();
		ByteQueue Request(ByteQueue);
};