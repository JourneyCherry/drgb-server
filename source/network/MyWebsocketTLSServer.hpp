#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include "MyTCPServer.hpp"
#include "MyWebsocketTLSClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;
class MyWebsocketTLSServer : public MyTCPServer
{
	private:
		boost::asio::ssl::context sslctx;
		
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		MyWebsocketTLSServer(int, int, std::string, std::string);
		MyWebsocketTLSServer(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer(MyWebsocketTLSServer&&) = delete;

		MyWebsocketTLSServer& operator=(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer& operator=(MyWebsocketTLSServer&&) = delete;
};