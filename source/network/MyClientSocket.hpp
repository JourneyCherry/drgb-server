#pragma once
#include <string>
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include "Expected.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "KeyExchanger.hpp"
#include "Encryptor.hpp"

using mylib::utils::Expected;
using mylib::utils::PacketProcessor;
using mylib::utils::ByteQueue;
using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;
using mylib::security::KeyExchanger;
using mylib::security::Encryptor;

class MyClientSocket : public std::enable_shared_from_this<MyClientSocket>
{
	private:
		bool isSecure;
		Encryptor encryptor, decryptor;
		KeyExchanger exchanger;

	protected:
		std::string Address;
		int Port;
		PacketProcessor recvbuffer;
		bool initiateClose;
		std::mutex mtx;
		
		std::queue<boost::asio::ip::basic_resolver_entry<boost::asio::ip::tcp>> endpoints;

		using timer_t = boost::asio::steady_timer;
		boost::asio::any_io_executor ioc_ref;
		std::unique_ptr<timer_t> timer;

		//For User of Client Socket
		std::function<void(std::shared_ptr<MyClientSocket>)> cleanHandler;
		Expected<ByteQueue> Recv();

		virtual void DoRecv(std::function<void(boost::system::error_code, size_t)>) = 0;
		virtual void GetRecv(size_t) = 0;
		virtual ErrorCode DoSend(const byte*, const size_t&) = 0;
		virtual void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) = 0;
		void Recv_Handle(std::shared_ptr<MyClientSocket>, boost::system::error_code, size_t, std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)>);
		virtual boost::asio::any_io_executor GetContext() = 0;
		virtual bool isReadable() const = 0;
		virtual void DoClose() = 0;

	public:
		static constexpr int TIME_KEYEXCHANGE = 2000;
		
		MyClientSocket();
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		virtual void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) = 0;
		void Connect(std::string, int, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>);
		virtual bool is_open() const = 0;
		void Close();

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		ErrorCode Send(ByteQueue);
		ErrorCode Send(std::vector<byte>);
		void SetTimeout(const int&, std::function<void(std::shared_ptr<MyClientSocket>)>);
		void SetCleanUp(std::function<void(std::shared_ptr<MyClientSocket>)>);
		void CancelTimeout();

		std::string ToString();
		std::string GetAddress();
		int GetPort();
		void StartRecv(std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)>);
		void KeyExchange(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>);

		static bool isNormalClose(const ErrorCode&);
};