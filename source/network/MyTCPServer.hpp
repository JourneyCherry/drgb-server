#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "MyServerSocket.hpp"
#include "MyTCPClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;
using mylib::utils::Expected;

class MyTCPServer : public MyServerSocket
{
	private:
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::acceptor acceptor;

	public:
		MyTCPServer(int);
		MyTCPServer(const MyTCPServer&) = delete;
		MyTCPServer(MyTCPServer&&) = delete;
		~MyTCPServer();

		Expected<std::shared_ptr<MyClientSocket>, ErrorCode> Accept() override;
		void Close() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};