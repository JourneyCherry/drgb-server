#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include "MyServerSocket.hpp"
#include "MyWebsocketTLSClient.hpp"
#include "ConfigParser.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;
using mylib::utils::ConfigParser;

class MyWebsocketTLSServer : public MyServerSocket
{
	private:
		boost::beast::net::io_context ioc;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ssl::context sslctx;

	public:
		MyWebsocketTLSServer(int);
		MyWebsocketTLSServer(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer(MyWebsocketTLSServer&&) = delete;
		~MyWebsocketTLSServer();

		Expected<std::shared_ptr<MyClientSocket>, ErrorCode> Accept() override;
		void Close() override;

		MyWebsocketTLSServer& operator=(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer& operator=(MyWebsocketTLSServer&&) = delete;
};