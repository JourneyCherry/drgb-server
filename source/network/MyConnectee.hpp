#pragma once
#include <functional>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <string>
#include "MyTCPServer.hpp"
#include "MyConnector.hpp"
#include "Logger.hpp"

using mylib::utils::ByteQueue;
using mylib::utils::StackTraceExcept;
using mylib::utils::ErrorCodeExcept;
using mylib::utils::Logger;
using mylib::utils::PacketProcessor;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;
using mylib::utils::ErrorCode;

class MyConnectee : public MyTCPServer
{
	private:
		ThreadExceptHandler except;

		std::map<std::string, std::function<ByteQueue(ByteQueue)>> KeywordProcessMap;
		std::string identify_keyword;

		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool ClientAuth(std::shared_ptr<MyClientSocket>, Expected<std::string, ErrorCode>);

	public:
		MyConnectee(std::string, int, int, ThreadExceptHandler*);
	public:
		void Accept(std::string, std::function<ByteQueue(ByteQueue)>);
		void Request(std::string, ByteQueue, std::function<void(std::shared_ptr<MyConnector>, Expected<ByteQueue, StackErrorCode>)>);
		size_t GetAuthorized();
		std::map<std::string, size_t> GetUsage();
};