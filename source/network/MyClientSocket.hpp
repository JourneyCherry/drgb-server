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

/**
 * @brief Client Socket Interface. 
 * **Spec**
 *   - Key Exchange : ECDH
 *   - Encryption : AES-256-CBC
 *   - Timer : Boost's steady timer
 * 
 * **Start Process**
 *   1. (If it's from client) Connect()
 *   2. Prepare() <- 'MyServerSocket::GetClient()' <- 'MyServerSocket::StartAccept()'
 *   3. KeyExchange() <- 'accept_handle' of 'MyServer::Start()'
 * 
 * **Recv Process**
 *   1. StartRecv() <- 'MyServer::AcceptProcess()' and 'MyServer::ClientProcess()'
 *   2. DoRecv()
 *   3. Recv_Handle()
 *   4. GetRecv()
 *   5. handler() -> return to '1.' until socket is closed
 * 
 * **Send Process**
 *   1. Send() <- 'MyServer::ClientProcess()' or else
 *   2. Encapsulate and DoSend()
 *   3. Send_Handle()
 *   4. if there are any bytes left, return to '2.'
 * 
 * **Close Process**
 *   1. DoClose() <- everywhere
 *   2. cleanHandler()
 * 
 */
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

		std::mutex send_mtx;
		std::queue<std::vector<byte>> send_queue;
		
		std::queue<boost::asio::ip::basic_resolver_entry<boost::asio::ip::tcp>> endpoints;

		int ttl;		//Time to live
		std::unique_ptr<boost::asio::steady_timer> timer;

		//For User of Client Socket
		std::function<void(std::shared_ptr<MyClientSocket>)> cleanHandler;
		Expected<ByteQueue> Recv();

		virtual void DoRecv(std::function<void(boost::system::error_code, size_t)>) = 0;
		virtual void GetRecv(size_t) = 0;
		virtual void DoSend(const byte*, const size_t&, std::function<void(boost::system::error_code, size_t)>) = 0;
		virtual void Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>, const boost::system::error_code&) = 0;
		void Recv_Handle(std::shared_ptr<MyClientSocket>, boost::system::error_code, size_t, std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)>);
		void Send_Handle(std::shared_ptr<MyClientSocket>, boost::system::error_code, size_t, std::vector<byte>);
		virtual boost::asio::any_io_executor GetContext() = 0;
		virtual bool isReadable() const = 0;
		virtual void DoClose() = 0;

	public:
		static constexpr int TIME_KEYEXCHANGE = 2000;
		
		MyClientSocket();
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		virtual void Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) = 0;	//Preparation for each socket type such as handshake
		void Connect(std::string, int, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>);
		virtual bool is_open() const = 0;
		void Close();

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		/**
		 * @brief Enqueue ByteQueue into Send Queue through 'DoSend()' and 'Send_Handle()'
		 * 
		 * @param bytes non-capsulated complete message
		 */
		void Send(ByteQueue bytes);
		/**
		 * @brief Enqueue bytes into Send Queue through 'DoSend()' and 'Send_Handle()'
		 * 
		 * @param bytes non-capsulated complete message
		 */
		void Send(std::vector<byte> bytes);
		/**
		 * @brief Set disposable Timer call 'callback' after 'ms' milliseconds. If you want to continuous timer, call this in 'callback'
		 * 
		 * @param ms millisecond
		 * @param callback process called just right after time passed.
		 */
		void SetTimeout(const int& ms, std::function<void(std::shared_ptr<MyClientSocket>)> callback);
		/**
		 * @brief Set 'cleanHandler'. 'cleanHandler' will be called just after the socket is closed.
		 * 
		 * @param callback process when the socket is closed.
		 */
		void SetCleanUp(std::function<void(std::shared_ptr<MyClientSocket>)> callback);
		/**
		 * @brief Cancel Timer. timer callback will not be called after this method.
		 * 
		 */
		void CancelTimeout();

		/**
		 * @brief Get Address string with port number like 'xxx.xxx.xxx.xxx:xxxxx'
		 * 
		 * @return std::string address string
		 */
		std::string ToString();
		/**
		 * @brief Get Address string only like 'xxx.xxx.xxx.xxx'
		 * 
		 * @return std::string address string
		 */
		std::string GetAddress();
		/**
		 * @brief Get port number
		 * 
		 * @return int port number
		 */
		int GetPort();
		/**
		 * @brief Start Receive Process. If a complete message is received or there is any error, handler will be called.
		 * 
		 * @param handler callback for message.
		 */
		void StartRecv(std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handler);
		/**
		 * @brief Start Key Exchange Process. the result will be through 'handler'.
		 * 
		 * @param handler callback for result.
		 */
		void KeyExchange(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler);

		static bool isNormalClose(const ErrorCode&);

		void FillTTL(const int&);
		bool CountTTL();
};