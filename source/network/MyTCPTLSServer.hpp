#pragma once
#include <boost/asio/ssl.hpp>
#include "MyTCPServer.hpp"
#include "MyTCPTLSClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::Expected;

class MyTCPTLSServer : public MyTCPServer
{
	private:
		boost::asio::ssl::context sslctx;
	
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		MyTCPTLSServer(int, int, std::string, std::string);
		MyTCPTLSServer(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer(MyTCPTLSServer&&) = delete;

		MyTCPTLSServer& operator=(const MyTCPTLSServer&) = delete;
		MyTCPTLSServer& operator=(MyTCPTLSServer&&) = delete;
};