#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <string>
#include "MyInvoker.hpp"
#include "MyMsg.hpp"
#include "MyExcepts.hpp"
#include "Thread.hpp"

class MyConnectee
{
	private:
		using Thread = mylib::threads::Thread;
		
		static constexpr byte SUCCESS = 0;
		static constexpr byte ERR_PROTOCOL_VIOLATION = 11;
	private:
		const unsigned int LISTENSIZE = 3;
		const unsigned int BUFSIZE = 1024;
		MyCommon::Invoker<int> server_fd;
		int port;
		int isRunning;
		int opt_reuseaddr;
		Thread t_accept;
		std::map<std::string, std::pair<std::shared_ptr<Thread>, std::shared_ptr<MyCommon::Invoker<int>>>> clients;
	public:
		MyConnectee(MyThreadExceptInterface*);
		~MyConnectee();
	public:
		void Open(int);
		void Close();
		void Accept(std::string, std::function<MyBytes(MyBytes)>);
	private:
		void AcceptLoop(std::shared_ptr<bool>);
		void ClientLoop(std::string, std::shared_ptr<MyCommon::Invoker<int>>, std::function<MyBytes(MyBytes)>, std::shared_ptr<bool>);
};