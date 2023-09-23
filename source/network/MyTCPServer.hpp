#pragma once
#include <boost/asio/ip/tcp.hpp>
#include "MyServerSocket.hpp"
#include "MyTCPClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;
using mylib::utils::Expected;

/**
 * @brief TCP Socket for Server.
 * 
 */
class MyTCPServer : public MyServerSocket
{
	protected:
		boost::asio::ip::tcp::acceptor acceptor;

		virtual std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>);
		void CloseSocket() override;

	public:
		/**
		 * @brief Constructor of TCP Server Socket
		 * 
		 * @param p port number of accept socket. If this value is 0, random open port number will be selected and you can get it by calling 'GetPort()' method.
		 * @param t thread number for io_context. It should be at least 1.
		 */
		MyTCPServer(int p, int t);
		MyTCPServer(const MyTCPServer&) = delete;
		MyTCPServer(MyTCPServer&&) = delete;
		~MyTCPServer();

		/**
		 * @brief Start a socket to accept clients.
		 * 
		 * @param handle handler called when a client has been successfully accepted.
		 */
		void StartAccept(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle) override;
		/**
		 * @brief is the socket opened?
		 * 
		 * @return true 
		 * @return false 
		 */
		bool is_open() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};