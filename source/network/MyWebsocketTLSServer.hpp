#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include "MyTCPServer.hpp"
#include "MyWebsocketTLSClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;

/**
 * @brief Websocket for Server using TLS
 * 
 */
class MyWebsocketTLSServer : public MyTCPServer
{
	private:
		boost::asio::ssl::context sslctx;
		
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		/**
		 * @brief Constructor of Websocket Server Socket using TLS
		 * 
		 * @param p port number of accept socket. If this value is 0, random open port number will be selected and you can get it by calling 'GetPort()' method.
		 * @param t thread number for io_context. It should be at least 1.
		 * @param cert_path file path of TLS certificate
		 * @param key_path file path of TLS key
		 */
		MyWebsocketTLSServer(int p, int t, std::string cert_path, std::string key_path);
		MyWebsocketTLSServer(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer(MyWebsocketTLSServer&&) = delete;

		MyWebsocketTLSServer& operator=(const MyWebsocketTLSServer&) = delete;
		MyWebsocketTLSServer& operator=(MyWebsocketTLSServer&&) = delete;
};