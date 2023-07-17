#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <queue>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyWebsocketTLSClient : public MyClientSocket
{
	private:
		using booststream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;
		static constexpr int TIME_HANDSHAKE = 1500;

		boost::beast::flat_buffer buffer;

		boost::asio::ssl::context sslctx;
		booststream ws;

		std::string DomainStr;

	protected:
		void DoRecv(std::function<void(boost::system::error_code, size_t)>) override;
		void GetRecv(size_t) override;
		ErrorCode DoSend(const byte*, const size_t&) override;
		void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) override;
		boost::asio::any_io_executor GetContext() override;
		bool isReadable() const override;
		void DoClose() override;

	public:
		MyWebsocketTLSClient(boost::asio::ssl::context&, boost::asio::ip::tcp::socket);
		MyWebsocketTLSClient(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient(MyWebsocketTLSClient&&) = delete;
		~MyWebsocketTLSClient() = default;

		void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool is_open() const override;

		MyWebsocketTLSClient& operator=(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient& operator=(MyWebsocketTLSClient&&) = delete;
};