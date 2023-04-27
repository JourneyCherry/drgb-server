#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <random>
#include <limits>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "MyTCPClient.hpp"
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
	private:
		std::string target_addr;
		int target_port;
		std::string target_keyword;
	
	private:
		static constexpr int TIME_WAIT_ANSWER = 1;
		static constexpr int RETRY_WAIT_SEC = 3;

		bool isConnected;
		bool keepTryConnect;
		std::shared_ptr<MyClientSocket> socket;
	
		Thread t_recv;
		std::function<ByteQueue(ByteQueue)> inquiry_process;
		std::mutex m_req;
		std::condition_variable cv_req;
		std::map<unsigned int, ByteQueue> answer_map;

	public:
		std::string keyword;

	private:
		std::condition_variable cv;
		void ConnectLoop();	//TODO : Coroutine으로 변경 필요.

	public:
		MyConnector(ThreadExceptHandler*, std::string, int, std::string, std::function<ByteQueue(ByteQueue)>);
		MyConnector(ThreadExceptHandler*, std::shared_ptr<MyClientSocket>);
		~MyConnector();

		void SetInquiry(std::function<ByteQueue(ByteQueue)>);
		void Accept(std::string);
		void Reject(errorcode_t);
		void Connect();
		void Close();
		void RecvLoop();	//TODO : Coroutine으로 변경 필요.
		Expected<ByteQueue, StackErrorCode> Request(ByteQueue);
		std::string ToString() const;
};