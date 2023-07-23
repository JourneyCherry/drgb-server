#pragma once
#include <boost/asio/ssl.hpp>
#include "MyClientSocket.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyTCPTLSClient : public MyClientSocket
{
	private:
		using tcpstream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
		
		static constexpr size_t BUF_SIZE = 1024;
		std::array<byte, BUF_SIZE> buffer;

		boost::asio::ssl::context sslctx;
		tcpstream ws;
	
	protected:
		void DoRecv(std::function<void(boost::system::error_code, size_t)>) override;
		void GetRecv(size_t) override;
		void DoSend(const byte*, const size_t&, std::function<void(boost::system::error_code, size_t)>) override;
		void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) override;
		boost::asio::any_io_executor GetContext() override;
		bool isReadable() const override;
		void DoClose() override;

	public:
		MyTCPTLSClient(boost::asio::ssl::context&, boost::asio::ip::tcp::socket);	//TCPServer에서 Accept한 socket으로 생성할 때의 생성자.
		MyTCPTLSClient(const MyTCPTLSClient&) = delete;
		MyTCPTLSClient(MyTCPTLSClient&&) = delete;
		~MyTCPTLSClient() = default;

		void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;
		bool is_open() const override;

		MyTCPTLSClient& operator=(const MyTCPTLSClient&) = delete;
		MyTCPTLSClient& operator=(MyTCPTLSClient&&) = delete;
};