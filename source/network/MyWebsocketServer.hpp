#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "MyTCPServer.hpp"
#include "MyWebsocketClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;

class MyWebsocketServer : public MyTCPServer
{
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		MyWebsocketServer(int, int);
		MyWebsocketServer(const MyWebsocketServer&) = delete;
		MyWebsocketServer(MyWebsocketServer&&) = delete;

		MyWebsocketServer& operator=(const MyWebsocketServer&) = delete;
		MyWebsocketServer& operator=(MyWebsocketServer&&) = delete;
};