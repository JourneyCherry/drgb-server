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
		using booststream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;
		static constexpr float TIME_HANDSHAKE = 1.5f;
		static constexpr float TIME_CLOSE = 1.5f;

		//TODO : Asynchronous function을 synchronous 하게 동작하기 위한 변수. 추후 async로 변경되면 삭제 필요.
		bool isAsyncDone;
		boost::beast::flat_buffer buffer;
		std::vector<byte> recv_bytes;
		ErrorCode recv_ec;
		////

		std::shared_ptr<boost::asio::io_context> pioc;
		boost::asio::ssl::context sslctx;
		booststream ws;

		std::chrono::steady_clock::duration timeout;
		boost::asio::steady_timer deadline;

	public:
		MyWebsocketTLSClient() : isAsyncDone(true), sslctx{boost::asio::ssl::context::tlsv12}, pioc(std::make_shared<boost::asio::io_context>()), ws(*pioc, std::ref(sslctx)), timeout(0), deadline(*pioc), MyClientSocket() {}
		MyWebsocketTLSClient(std::shared_ptr<boost::asio::io_context>, boost::asio::ssl::context&, boost::asio::ip::tcp::socket);
		MyWebsocketTLSClient(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient(MyWebsocketTLSClient&&) = delete;
		~MyWebsocketTLSClient();
		
		StackErrorCode Connect(std::string, int) override;
		void Close() override;
		void SetTimeout(float = 0.0f) override;

		MyWebsocketTLSClient& operator=(const MyWebsocketTLSClient&) = delete;
		MyWebsocketTLSClient& operator=(MyWebsocketTLSClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
};