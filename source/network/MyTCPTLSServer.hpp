#pragma once
#include <boost/asio/ssl.hpp>
#include "MyTCPServer.hpp"
#include "MyTCPTLSClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::Expected;
/**
 * @brief TCP Socket for Server using TLS
 * 
 */
class MyTCPTLSServer : public MyTCPServer
{
	private:
		boost::asio::ssl::context sslctx;
	
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		/**
		 * @brief Constructor of TCP Server Socket using TLS
		 * 
		 * @param p port number of accept socket. If this value is 0, random open port number will be selected and you can get it by calling 'GetPort()' method.
		 * @param t thread number for io_context. It should be at least 1.
		 * @param cert_path file path of TLS certificate
		 * @param key_path file path of TLS key
		 */
		MyTCPTLSServer(int p, int t, std::string cert_path, std::string key_path);
		MyTCPTLSServer(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer(MyTCPTLSServer&&) = delete;

		MyTCPTLSServer& operator=(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer& operator=(MyTCPTLSServer&&) = delete;
};