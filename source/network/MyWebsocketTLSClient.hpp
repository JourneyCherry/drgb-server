#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyWebsocketTLSClient : public MyClientSocket
{
	private:
		using booststream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>;

		std::shared_ptr<boost::asio::io_context> pioc;
		std::unique_ptr<booststream> ws;

	public:
		MyWebsocketTLSClient() : MyClientSocket() {}
		MyWebsocketTLSClient(std::shared_ptr<boost::asio::io_context>, boost::asio::ssl::context&, boost::asio::ip::tcp::socket);
		MyWebsocketTLSClient(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient(MyWebsocketTLSClient&&) = delete;
		~MyWebsocketTLSClient();
		
		StackErrorCode Connect(std::string, int) override;
		void Close() override;

		MyWebsocketTLSClient& operator=(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient& operator=(MyWebsocketTLSClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
};