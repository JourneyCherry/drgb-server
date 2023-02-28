#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <string>
#include "Invoker.hpp"
#include "PacketProcessor.hpp"
#include "ThreadExceptHandler.hpp"
#include "Thread.hpp"
#include "Logger.hpp"

using mylib::utils::Invoker;
using mylib::utils::ByteQueue;
using mylib::utils::StackTraceExcept;
using mylib::utils::Logger;
using mylib::utils::PacketProcessor;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;

class MyConnectee
{
	private:		
		static constexpr byte SUCCESS = 0;
		static constexpr byte ERR_PROTOCOL_VIOLATION = 11;
	private:
		const unsigned int LISTENSIZE = 3;
		const unsigned int BUFSIZE = 1024;
		Invoker<int> server_fd;
		int port;
		int isRunning;
		int opt_reuseaddr;
		Thread t_accept;
		std::map<std::string, std::pair<std::shared_ptr<Thread>, std::shared_ptr<Invoker<int>>>> clients;
	public:
		MyConnectee(ThreadExceptHandler*);
		~MyConnectee();
	public:
		void Open(int);
		void Close();
		void Accept(std::string, std::function<ByteQueue(ByteQueue)>);
	private:
		void AcceptLoop(std::shared_ptr<bool>);
		void ClientLoop(std::string, std::shared_ptr<Invoker<int>>, std::function<ByteQueue(ByteQueue)>, std::shared_ptr<bool>);
};