#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyWebsocketClient : public MyClientSocket
{
	private:
		std::shared_ptr<boost::asio::io_context> pioc;
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;

	public:
		MyWebsocketClient(std::shared_ptr<boost::asio::io_context>, boost::asio::ip::tcp::socket);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient();
		
		Expected<ByteQueue, ErrorCode> Recv() override;
		ErrorCode Send(ByteQueue) override;
		void Close() override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;
};