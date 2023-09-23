#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <queue>
#include "MyClientSocket.hpp"
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::ErrorCodeExcept;

/**
 * @brief Websocket Socket for Client.
 * 
 */
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
		void DoSend(const byte*, const size_t&, std::function<void(boost::system::error_code, size_t)>) override;
		void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) override;
		boost::asio::any_io_executor GetContext() override;
		bool isReadable() const override;
		void DoClose() override;

	public:
		/**
		 * @brief Constructor of Websocket Client Socket.
		 * 
		 * @param socket_ TCP Socket from acceptor(Server) or io_context(Independent Client)
		 */
		MyWebsocketClient(boost::asio::ip::tcp::socket socket_);
		MyWebsocketClient(const MyWebsocketClient&) = delete;
		MyWebsocketClient(MyWebsocketClient&&) = delete;
		~MyWebsocketClient() = default;

		void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;	//Do Websocket Handshake
		/**
		 * @brief Is the socket opened?
		 * 
		 * @return true 
		 * @return false 
		 */
		bool is_open() const override;

		MyWebsocketClient& operator=(const MyWebsocketClient&) = delete;
		MyWebsocketClient& operator=(MyWebsocketClient&&) = delete;
};