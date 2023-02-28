#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include "MyClientSocket.hpp"

using mylib::utils::StackTraceExcept;

class MyWebsocketClient : public MyClientSocket
{
	private:
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;

	public:
		MyWebsocketClient(boost::asio::ip::tcp::socket);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient();
		
		Expected<ByteQueue, int> Recv() override;
		Expected<int> Send(ByteQueue) override;
		void Close() override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;
};