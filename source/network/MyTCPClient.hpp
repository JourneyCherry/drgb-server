#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include "MyClientSocket.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

class MyTCPClient : public MyClientSocket
{
	private:
		using tcpstream = boost::asio::ip::tcp::socket;
		
		//TODO : Asynchronous function을 synchronous 하게 동작하기 위한 변수. 추후 async로 변경되면 삭제 필요.
		bool isAsyncDone;
		ErrorCode recv_ec;
		//boost::asio::mutable_buffer buffer;
		std::vector<byte> result_bytes;
		static constexpr size_t BUF_SIZE = 1024;
		std::array<byte, BUF_SIZE> buffer;
		////

		std::shared_ptr<boost::asio::io_context> pioc;
		boost::asio::ip::tcp::socket socket;
	
		std::chrono::steady_clock::duration timeout;
		boost::asio::steady_timer deadline;

	public:
		MyTCPClient();
		MyTCPClient(std::shared_ptr<boost::asio::io_context>, boost::asio::ip::tcp::socket);	//TCPServer에서 Accept한 socket으로 생성할 때의 생성자.
		MyTCPClient(const MyTCPClient&) = delete;
		MyTCPClient(MyTCPClient&&) = delete;
		~MyTCPClient();
	public:
		StackErrorCode Connect(std::string, int) override;
		void Close() override;
		void SetTimeout(float = 0.0f) override;

		MyTCPClient& operator=(const MyTCPClient&) = delete;
		MyTCPClient& operator=(MyTCPClient&&) = delete;

	private:
		Expected<std::vector<byte>, ErrorCode> RecvRaw() override;
		ErrorCode SendRaw(const byte*, const size_t&) override;
};