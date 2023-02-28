#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "MyServerSocket.hpp"
#include "MyWebsocketClient.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;

class MyWebsocketServer : public MyServerSocket
{
	private:
		boost::beast::net::io_context ioc;
		boost::asio::ip::tcp::acceptor acceptor;

	public:
		MyWebsocketServer(int);
		MyWebsocketServer(const MyWebsocketServer&) = delete;
		MyWebsocketServer(MyWebsocketServer&&) = delete;
		~MyWebsocketServer();

		Expected<std::shared_ptr<MyClientSocket>> Accept() override;
		void Close() override;

		MyWebsocketServer& operator=(const MyWebsocketServer&) = delete;
		MyWebsocketServer& operator=(MyWebsocketServer&&) = delete;
};