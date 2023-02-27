#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "MyServerSocket.hpp"
#include "MyWebsocketClient.hpp"

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

		MyExpected<std::shared_ptr<MyClientSocket>, int> Accept() override;
		void Close() override;

		MyWebsocketServer& operator=(const MyWebsocketServer&) = delete;
		MyWebsocketServer& operator=(MyWebsocketServer&&) = delete;
};