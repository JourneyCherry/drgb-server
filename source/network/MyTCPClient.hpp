#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <queue>
#include "MyClientSocket.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyTCPClient : public MyClientSocket
{
	private:
		using tcpstream = boost::asio::ip::tcp::socket;
		
		static constexpr size_t BUF_SIZE = 1024;
		std::array<byte, BUF_SIZE> buffer;

		boost::asio::ip::tcp::socket socket;

	protected:
		void DoRecv(std::function<void(boost::system::error_code, size_t)>) override;
		void GetRecv(size_t) override;
		ErrorCode DoSend(const byte*, const size_t&) override;
		void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) override;
		boost::asio::any_io_executor GetContext() override;
		bool isReadable() const override;
		void DoClose() override;

	public:
		MyTCPClient(boost::asio::ip::tcp::socket);
		MyTCPClient(const MyTCPClient&) = delete;
		MyTCPClient(MyTCPClient&&) = delete;
		~MyTCPClient() = default;

		virtual void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool is_open() const override;
		
		MyTCPClient& operator=(const MyTCPClient&) = delete;
		MyTCPClient& operator=(MyTCPClient&&) = delete;
};