#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/thread_pool.hpp>
#include <vector>
#include "ThreadExceptHandler.hpp"
#include "MyConnector.hpp"
#include "DeMap.hpp"

using mylib::threads::ThreadExceptHandler;
using mylib::utils::DeMap;

class MyConnectorPool : public ThreadExceptHandler
{
	private:
		static constexpr int RETRY_WAIT_SEC = 3000;		//ms

		bool isRunning;
		boost::asio::io_context ioc;
		std::string keyword;
		std::vector<std::string> keywords;
		DeMap<std::string, std::tuple<std::string, int>, std::shared_ptr<MyConnector>> pool;

		boost::asio::thread_pool threadloop;
		void Run();
		void Reconnect(std::shared_ptr<MyClientSocket>, std::string, std::function<ByteQueue(ByteQueue)>);

	public:
		MyConnectorPool(std::string, const int&, ThreadExceptHandler* = nullptr);
		~MyConnectorPool();
		void Connect(std::string, int, std::string, std::function<ByteQueue(ByteQueue)>);
		Expected<ByteQueue, StackErrorCode> Request(std::string, ByteQueue);
		void Close();
		size_t GetConnected() const;
		size_t GetAuthorized();
		std::map<std::string, int> GetUsage();
};