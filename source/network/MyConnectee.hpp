#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <string>
#include "MyTCPServer.hpp"
#include "MyWebsocketServer.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "VariadicPool.hpp"
#include "MyConnector.hpp"
#include "Logger.hpp"

using mylib::utils::ByteQueue;
using mylib::utils::StackTraceExcept;
using mylib::utils::ErrorCodeExcept;
using mylib::utils::Logger;
using mylib::utils::PacketProcessor;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;
using mylib::threads::VariadicPool;
using mylib::utils::ErrorCode;

class MyConnectee : public ThreadExceptHandler
{
	private:		
		static constexpr byte SUCCESS = 0;
		static constexpr byte ERR_PROTOCOL_VIOLATION = 11;
	private:
		//MyTCPServer server_socket;
		MyTCPServer server_socket;
		int isRunning;
		Thread t_accept;
		std::map<std::string, std::function<ByteQueue(ByteQueue)>> KeywordProcessMap;
		std::map<std::string, std::shared_ptr<VariadicPool<std::shared_ptr<MyConnector>>>> ConnectorsMap;	//TODO : VariadicPool을 Coroutine으로 변경하기.
		std::string identify_keyword;
	public:
		MyConnectee(std::string, int, ThreadExceptHandler*);
		~MyConnectee();
	public:
		void Open();
		void Close();
		void Accept(std::string, std::function<ByteQueue(ByteQueue)>);
		void Request(std::string, ByteQueue, std::function<void(std::shared_ptr<MyConnector>, ByteQueue)>);
	private:
		void AcceptLoop();
};