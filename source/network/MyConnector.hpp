#pragma once
#include <string>
#include <chrono>
#include <functional>
#include <future>
#include <mutex>
#include <random>
#include <limits>
#include "MyTCPClient.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "Thread.hpp"
#include "Logger.hpp"
#include "Expected.hpp"

using mylib::utils::ByteQueue;
using mylib::utils::StackErrorCode;
using mylib::utils::StackTraceExcept;
using mylib::utils::PacketProcessor;
using mylib::utils::Logger;
using mylib::threads::Thread;
using mylib::threads::ThreadExceptHandler;
using mylib::utils::Expected;

class MyConnector : public MyTCPClient
{
	private:
		bool m_authorized;
		static constexpr int TIME_WAIT_ANSWER = 5000;	//ms

		std::function<ByteQueue(ByteQueue)> inquiry_process;
		std::mutex m_req;
		std::map<unsigned int, std::promise<ByteQueue>> answer_map;

		void RecvHandler(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);

	public:
		std::string keyword;

	private:
		void KeyExchange_Handle(std::shared_ptr<MyClientSocket>, ErrorCode);
		void ConnectorEnter(std::shared_ptr<MyClientSocket>, ErrorCode);

	public:
		MyConnector(boost::asio::ip::tcp::socket);
		~MyConnector();

		void StartInquiry(std::function<ByteQueue(ByteQueue)>);
		void Connectee(std::function<bool(std::shared_ptr<MyClientSocket>, Expected<std::string, ErrorCode>)>);
		void Connector(std::string, std::function<void(std::shared_ptr<MyClientSocket>, Expected<std::string, ErrorCode>)>);
		Expected<ByteQueue, StackErrorCode> Request(ByteQueue);

		bool isAuthorized() const;
};