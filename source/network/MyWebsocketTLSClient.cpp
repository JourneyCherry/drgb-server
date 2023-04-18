#include "MyWebsocketTLSClient.hpp"

MyWebsocketTLSClient::MyWebsocketTLSClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ssl::context &sslctx, boost::asio::ip::tcp::socket socket)
 : pioc(_pioc), ws(nullptr), MyClientSocket()
{
	boost::beast::error_code ec;

	ws = std::make_unique<booststream>(std::move(socket), sslctx);
	auto &sock = boost::beast::get_lowest_layer(*ws);
	boost::asio::ip::tcp::endpoint ep = sock.remote_endpoint();
	Address = ep.address().to_string() + ":" + std::to_string(ep.port());

	SetTimeout(TIME_HANDSHAKE);
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
	SetTimeout();

	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws->binary(true);
}

MyWebsocketTLSClient::~MyWebsocketTLSClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyWebsocketTLSClient::RecvRaw()
{
	if(!ws->is_open())
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	std::vector<byte> result_bytes;
	ErrorCode ec;

	if(isAsyncDone)
	{
		isAsyncDone = false;
		ws->async_read(buffer, [&](boost::beast::error_code m_ec, size_t bytes_written)
		{
			isAsyncDone = true;
			ec = ErrorCode(m_ec);
			if(!ec)
				return;
			if(bytes_written == 0)
			{
				ec = ErrorCode(ERR_CONNECTION_CLOSED);
				return;
			}
			if(!ws->got_binary())
			{
				ec = ErrorCode(ERR_PROTOCOL_VIOLATION);
				return;
			}

			unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
			size_t size = boost::asio::buffer_size(buffer.data());
			buffer.clear();

			result_bytes.assign(first, first + size);
		});
	}

	if(pioc->stopped())
		pioc->restart();
	pioc->run();

	if(!ec)
	{
		if(isNormalClose(ec))
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		return ec;
	}

	return result_bytes;
}

ErrorCode MyWebsocketTLSClient::SendRaw(const byte* bytes, const size_t &len)
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

StackErrorCode MyWebsocketTLSClient::Connect(std::string addr, int port)
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
		return {ErrorCode(ERR_get_error()), __STACKINFO__};
	
	SetTimeout(TIME_HANDSHAKE);
	ws->next_layer().handshake(boost::asio::ssl::stream_base::client, ec);
	SetTimeout();
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

void MyWebsocketTLSClient::SetTimeout(float sec)
{
	auto timeout = boost::beast::websocket::stream_base::none();
	if(sec > 0.0f)
	{
		long millisec = std::floor(sec * 1000);
		timeout = std::chrono::milliseconds(millisec);
	}
	boost::beast::websocket::stream_base::timeout opt;
	opt.handshake_timeout = timeout;
	opt.idle_timeout = timeout;
	opt.keep_alive_pings = true;
	ws->set_option(opt);
}

void MyWebsocketTLSClient::Close()
{
	std::exception_ptr eptr = nullptr;
	if(ws->is_open())
	{
		SetTimeout(TIME_CLOSE);
		ws->async_close(boost::beast::websocket::close_code::normal, [&](boost::beast::error_code err)
		{
			ErrorCode ec{err};
			if(!ec)
			{
				if(!isNormalClose(ec))
					throw ErrorCodeExcept(ec, __STACKINFO__);
			}
		});
	}
	if(pioc)
	{
		if(pioc->stopped())
		{
			try
			{
				pioc->restart();
				pioc->run();
			}
			catch(...)
			{
				eptr = std::current_exception();
			}
		}
	}
	recvbuffer.Clear();
	if(eptr)
		std::rethrow_exception(eptr);
}