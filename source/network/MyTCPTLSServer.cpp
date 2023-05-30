#include "MyTCPTLSServer.hpp"

MyTCPTLSServer::MyTCPTLSServer(int p, int t, std::string cert_path, std::string key_path) : sslctx{boost::asio::ssl::context::tlsv12}, MyTCPServer(p, t)
{
	//sslctx에 인증서, 비밀키 로드
	boost::beast::error_code ec;
	sslctx.set_options(boost::asio::ssl::context::default_workarounds);
	sslctx.use_certificate_file(cert_path, boost::asio::ssl::context::pem, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);
	sslctx.use_private_key_file(key_path, boost::asio::ssl::context::pem, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);

	/*
	//클라이언트에게 인증서 요구하기.
	sslctx.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);
	*/
}

std::shared_ptr<MyClientSocket> MyTCPTLSServer::GetClient(boost::asio::ip::tcp::socket& socket, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle)
{
	auto client = std::make_shared<MyTCPTLSClient>(sslctx, std::move(socket));
	client->Prepare(handle);

	return client;
}