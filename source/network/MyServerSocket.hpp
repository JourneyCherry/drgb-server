#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include "MyClientSocket.hpp"

using mylib::utils::Expected;

class MyServerSocket
{
	private:
		int thread_count;
		void watcher(std::shared_ptr<MyClientSocket>);
		void RemoveClient(std::shared_ptr<MyClientSocket>);

	protected:
		int port;
		boost::asio::thread_pool threadpool;
		boost::asio::io_context ioc;
		std::mutex mtx_client;
		std::vector<std::shared_ptr<MyClientSocket>> clients;

		void AddClient(std::shared_ptr<MyClientSocket>);
		std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> enterHandle;
		virtual void CloseSocket() = 0;
		void StartThread();

	public:
		MyServerSocket(int p, int t) : port(p), ioc{(t>0)?t:0}, threadpool((t>1)?t:1), thread_count((t>1)?t:1) {}
		MyServerSocket(const MyServerSocket&) = delete;
		MyServerSocket(MyServerSocket&&) = delete;
		virtual ~MyServerSocket() = default;

		MyServerSocket& operator=(const MyServerSocket&) = delete;
		MyServerSocket& operator=(MyServerSocket&&) = delete;

		virtual void StartAccept(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> = nullptr) = 0;
		virtual bool is_open() = 0;
		bool isRunnable() const;
		void Close();

		void safe_loop(std::function<void(std::shared_ptr<MyClientSocket>)>);
		int GetPort() const;
		size_t GetConnected() const;
};