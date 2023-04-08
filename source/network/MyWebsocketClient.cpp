#include "MyWebsocketClient.hpp"

MyWebsocketClient::MyWebsocketClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ssl::context &sslctx, boost::asio::ip::tcp::socket socket)
 : pioc(_pioc), ws(nullptr), MyClientSocket()
{
	boost::beast::error_code ec;

	ws = std::make_unique<booststream>(std::move(socket), sslctx);
	auto &sock = boost::beast::get_lowest_layer(*ws);
	boost::asio::ip::tcp::endpoint ep = sock.remote_endpoint();
	Address = ep.address().to_string() + ":" + std::to_string(ep.port());

	ws->next_layer().handshake(boost::asio::ssl::stream_base::server, ec);
	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
	}));

	ws->accept(ec);
	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws->binary(true);
}

MyWebsocketClient::~MyWebsocketClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyWebsocketClient::RecvRaw()
{
	if(!ws->is_open())
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	std::vector<byte> result_bytes;
	boost::beast::error_code result;
	boost::beast::flat_buffer buffer;

	ws->async_read(buffer, [&](boost::beast::error_code ec, size_t bytes_written)
	{
		result = ec;
		if(ec)
			return;
		if(bytes_written == 0)
			return;
		if(!ws->got_binary())
			return;

		unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
		size_t size = boost::asio::buffer_size(buffer.data());

		result_bytes.assign(first, first + size);
	});

	if(pioc->stopped())
		pioc->restart();
	pioc->run();

	if(!ws->got_binary())
		return {ErrorCode{ERR_PROTOCOL_VIOLATION}};
	if(
		result == boost::beast::websocket::error::closed || 
		result == boost::asio::error::eof ||
		result == boost::asio::error::operation_aborted
	)
		return {ErrorCode{ERR_CONNECTION_CLOSED}};
	if(result)
		return {ErrorCode{result}};

	return result_bytes;
}

ErrorCode MyWebsocketClient::SendRaw(const byte* bytes, const size_t &len)
{
	boost::asio::const_buffer buffer(bytes, len);
	boost::beast::error_code ec;

	if(!ws->is_open())
		return {ERR_CONNECTION_CLOSED};
		
	size_t sendlen = ws->write(buffer, ec);
	if(ec)
		return {ec};
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};

	return {SUCCESS};
}

StackErrorCode MyWebsocketClient::Connect(std::string addr, int port)
{
	Close();

	boost::beast::error_code ec;

	pioc = std::make_shared<boost::asio::io_context>();
	boost::asio::ssl::context sslctx(boost::asio::ssl::context::tlsv12_client);

	/*
	//sslctx에 인증서, 비밀키 로드
	sslctx.use_certificate_file(ConfigParser::GetString("CERT_FILE"), boost::asio::ssl::context::pem, ec);
	if(ec)
		return {ec, __STACKINFO__};
	sslctx.use_private_key_file(ConfigParser::GetString("KEY_FILE"), boost::asio::ssl::context::pem, ec);
	if(ec)
		return {ec, __STACKINFO__};
	*/

	boost::asio::ip::tcp::socket socket(*pioc);
	ws = std::make_unique<booststream>(*pioc, sslctx);
	//ws = std::make_unique<booststream>(std::move(socket));
	boost::asio::ip::tcp::resolver resolver(*pioc);

	auto const results = resolver.resolve(addr, std::to_string(port), ec);
	if(ec)
		return {ec, __STACKINFO__};
	auto endpoint = boost::beast::net::connect(boost::beast::get_lowest_layer(*ws), results);
	
	if(!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), addr.c_str()))
		return {GetSSLError(), __STACKINFO__};
	
	ws->next_layer().handshake(boost::asio::ssl::stream_base::client, ec);
	if(ec)
		return {ec, __STACKINFO__};

	ws->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-sync");
	}));

	ws->handshake(addr, "/", ec);
	if(ec)
		return {ec, __STACKINFO__};

	Address = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());

	ws->binary(true);

	return {};
}

void MyWebsocketClient::Close()
{
	if(ws && ws->is_open())
	{
		boost::beast::error_code ec;
		ws->close(boost::beast::websocket::close_code::normal, ec);	//TODO : 여기서 Block이 되면 모든 로직이 멈춘다. Timeout이나 비동기 종료가 필요함.
		if(
			ec == boost::beast::websocket::error::closed || 
			ec == boost::asio::error::eof ||
			ec == boost::asio::error::operation_aborted
		)
			return;
		if(ec)
			throw ErrorCodeExcept(ec, __STACKINFO__);
	}
	if(pioc)
		pioc->stop();
}