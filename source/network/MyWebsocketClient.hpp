#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyWebsocketClient : public MyClientSocket
{
	private:
		using booststream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
		static constexpr float TIME_HANDSHAKE = 1.5f;
		static constexpr float TIME_CLOSE = 1.5f;

		//TODO : Asynchronous function을 synchronous 하게 동작하기 위한 변수. 추후 async로 변경되면 삭제 필요.
		bool isAsyncDone;
		boost::beast::flat_buffer buffer;
		////

		std::shared_ptr<boost::asio::io_context> pioc;
		std::unique_ptr<booststream> ws;

	public:
		MyWebsocketClient() : MyClientSocket() {}
		MyWebsocketClient(std::shared_ptr<boost::asio::io_context>, boost::asio::ip::tcp::socket);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient();
		
		StackErrorCode Connect(std::string, int) override;
		void Close() override;
		void SetTimeout(float = 0.0f) override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
};