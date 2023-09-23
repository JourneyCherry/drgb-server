#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include "MyClientSocket.hpp"

using mylib::utils::Expected;

/**
 * @brief Server Socket Interface. It runs 'io_context' with t threads.
 * 
 */
class MyServerSocket
{
	private:
		int thread_count;
		void RemoveClient(std::shared_ptr<MyClientSocket>);

	protected:
		int port;
		boost::asio::thread_pool threadpool;
		boost::asio::io_context ioc;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
		std::mutex mtx_client;
		std::vector<std::shared_ptr<MyClientSocket>> clients;

		/**
		 * @brief Add 'MyClientSocket' into connected client list.
		 * 
		 * @param client client connected through accept socket.
		 */
		void AddClient(std::shared_ptr<MyClientSocket> client);
		virtual void CloseSocket() = 0;
		void StartThread();

	public:
		/**
		 * @brief Constructor of MyServerSocket Interface
		 * 
		 * @param p port number of accept socket. If this value is 0, random open port number will be selected and you can get it by calling 'GetPort()' method.
		 * @param t thread number for io_context. It should be at least 1.
		 */
		MyServerSocket(int p, int t) : port(p), ioc{(t>0)?t:0}, work_guard(boost::asio::make_work_guard(ioc)), threadpool((t>1)?t:1), thread_count((t>1)?t:1) {}
		MyServerSocket(const MyServerSocket&) = delete;
		MyServerSocket(MyServerSocket&&) = delete;
		virtual ~MyServerSocket() = default;

		MyServerSocket& operator=(const MyServerSocket&) = delete;
		MyServerSocket& operator=(MyServerSocket&&) = delete;

		virtual void StartAccept(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> = nullptr) = 0;
		virtual bool is_open() = 0;
		/**
		 * @brief Close Accept Socket.
		 * 
		 */
		void Close();

		/**
		 * @brief work for all connected ClientSocket thread-safely.
		 * 
		 * @param process work function.
		 */
		void safe_loop(std::function<void(std::shared_ptr<MyClientSocket>)> process);
		/**
		 * @brief Get the Port number of accept socket that is currently used.
		 * 
		 * @return int the number of port
		 */
		int GetPort() const;
		/**
		 * @brief Get the number of ClientSocket that is connected.
		 * 
		 * @return size_t the number of ClientSocket.
		 */
		size_t GetConnected() const;
};