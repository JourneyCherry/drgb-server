#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include "MyClientSocket.hpp"
#include "MyLogger.hpp"

class MyWebsocketClient : public MyClientSocket
{
	private:
		std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws;

	public:
		MyWebsocketClient(boost::asio::ip::tcp::socket);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient();
		
		MyExpected<MyBytes, int> Recv() override;
		MyExpected<int> Send(MyBytes) override;
		void Close() override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;
};