#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <queue>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyWebsocketClient : public MyClientSocket
{
	private:
		using booststream = boost::beast::websocket::stream<boost::beast::tcp_stream>;
		static constexpr int TIME_HANDSHAKE = 1500;
		boost::beast::flat_buffer buffer;
		booststream ws;

		std::string DomainStr;
	
	protected:
		void DoRecv(std::function<void(boost::system::error_code, size_t)>) override;
		void GetRecv(size_t) override;
		ErrorCode DoSend(const byte*, const size_t&) override;
		void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) override;
		boost::asio::any_io_executor GetContext() override;
		bool isReadable() const override;
		void Cancel() override;
		void Shutdown() override;

	public:
		MyWebsocketClient(boost::asio::ip::tcp::socket);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient();

		void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool is_open() const override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;
};