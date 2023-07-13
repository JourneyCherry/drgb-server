#pragma once
#include <boost/asio/ip/tcp.hpp>
#include "MyServerSocket.hpp"
#include "MyTCPClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;
using mylib::utils::Expected;

class MyTCPServer : public MyServerSocket
{
	protected:
		boost::asio::ip::tcp::acceptor acceptor;

		virtual std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>);
		void CloseSocket() override;

	public:
		MyTCPServer(int, int);
		MyTCPServer(const MyTCPServer&) = delete;
		MyTCPServer(MyTCPServer&&) = delete;
		~MyTCPServer();

		void StartAccept(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool is_open() override;

		MyTCPServer& operator=(const MyTCPServer&) = delete;
		MyTCPServer& operator=(MyTCPServer&&) = delete;
};