#include "MyWebsocketServer.hpp"

MyWebsocketServer::MyWebsocketServer(int p) : ioc(), acceptor{ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port)}, sslctx{boost::asio::ssl::context::tlsv12}, MyServerSocket(p)
{
	acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	//sslctx에 인증서, 비밀키 로드
	boost::beast::error_code ec;
	sslctx.use_certificate_file(ConfigParser::GetString("SSL_CERT"), boost::asio::ssl::context::pem, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);
	sslctx.use_private_key_file(ConfigParser::GetString("SSL_KEY"), boost::asio::ssl::context::pem, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);

	/*
	//클라이언트에게 인증서 요구하기.
	sslctx.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert, ec);
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);
	*/
}

MyWebsocketServer::~MyWebsocketServer()
{
	Close();
}

Expected<std::shared_ptr<MyClientSocket>, ErrorCode> MyWebsocketServer::Accept()
{
	std::shared_ptr<boost::asio::io_context> client_ioc = std::make_shared<boost::asio::io_context>();
	boost::asio::ip::tcp::socket socket{*client_ioc};
	std::shared_ptr<MyWebsocketClient> client = nullptr;
	boost::system::error_code result = boost::asio::error::operation_aborted;
	std::exception_ptr eptr = nullptr;

	acceptor.async_accept(socket, [&](boost::system::error_code ec){
		result = ec;
		if(ec)
			return;
		try
		{
			client = std::make_shared<MyWebsocketClient>(client_ioc, std::ref(sslctx), std::move(socket));
		}
		catch(StackTraceExcept e)
		{
			e.stack(__STACKINFO__);
			eptr = std::make_exception_ptr(e);
		}
		catch(const std::exception &e)
		{
			eptr = std::make_exception_ptr(StackTraceExcept(e.what(), __STACKINFO__));
		}
	});

	if(ioc.stopped())
		ioc.restart();
	ioc.run();

	if(result)
	{
		if(result == boost::asio::error::operation_aborted)
			return {ERR_CONNECTION_CLOSED};
		return {result};
	}
	if(eptr)
		std::rethrow_exception(eptr);
	if(!client)		//MyWebsocketServer::Close()의 호출로 종료되면 보통 위의 result에서 걸려 나가지만, 혹시모를 예외사항을 위해 
		return {result};//한번 더 검증을 수행함.
	return {client, true};
}

void MyWebsocketServer::Close()
{
	ioc.stop();
}