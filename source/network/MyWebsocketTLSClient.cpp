#include "MyWebsocketTLSClient.hpp"

MyWebsocketTLSClient::MyWebsocketTLSClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ssl::context &sslctx_, boost::asio::ip::tcp::socket socket)
 : pioc(_pioc), sslctx(boost::asio::ssl::context::tlsv12), ws(std::move(socket), sslctx_), isAsyncDone(true), timeout(0), deadline(*pioc), MyClientSocket()
{
	boost::beast::error_code ec;

	auto &sock = boost::beast::get_lowest_layer(ws).socket();
	boost::asio::ip::tcp::endpoint ep = sock.remote_endpoint();
	Address = ep.address().to_string();
	Port = ep.port();

	boost::beast::websocket::stream_base::timeout handshake_timeout;
	handshake_timeout.handshake_timeout = std::chrono::milliseconds((int)std::floor(TIME_HANDSHAKE * 1000));
	handshake_timeout.idle_timeout = boost::beast::websocket::stream_base::none();
	handshake_timeout.keep_alive_pings = false;
	ws.set_option(handshake_timeout);
	ws.next_layer().handshake(boost::asio::ssl::stream_base::server, ec);
	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
	}));

	ws.accept(ec);

	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws.binary(true);
	sock.non_blocking(false);
}

MyWebsocketTLSClient::~MyWebsocketTLSClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyWebsocketTLSClient::RecvRaw()
{
	if(!ws.is_open())
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	if(isAsyncDone)
	{
		isAsyncDone = false;
		recv_bytes.clear();
		recv_ec = ErrorCode(SUCCESS);
		if(timeout.count() > 0)
		{
			deadline.expires_after(timeout);
			deadline.async_wait([&](boost::beast::error_code error)
			{
				if(error == boost::asio::error::operation_aborted)
					return;
				ws.next_layer().next_layer().cancel();
			});
		}
		ws.async_read(buffer, [&](boost::beast::error_code m_ec, size_t bytes_written)
		{
			isAsyncDone = true;
			if(m_ec == boost::asio::error::operation_aborted)	//operation aborted가 발생하는 경우 : Timeout(from deadline), close(from MyWebsocketTLSClient::Close())
				recv_ec = ErrorCode(ERR_TIMEOUT);
			else
				recv_ec = ErrorCode(m_ec);
			if(!recv_ec)
				return;
			if(bytes_written == 0)
			{
				recv_ec = ErrorCode(ERR_CONNECTION_CLOSED);
				return;
			}
			if(!ws.got_binary())
			{
				recv_ec = ErrorCode(ERR_PROTOCOL_VIOLATION);
				return;
			}

			unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
			size_t size = boost::asio::buffer_size(buffer.data());
			buffer.clear();

			recv_bytes.assign(first, first + size);

			boost::system::error_code cancel_ec;
			deadline.cancel(cancel_ec);
			if(cancel_ec)
				recv_ec = ErrorCode(cancel_ec);
		});
	}

	if(pioc->stopped())
		pioc->restart();
	pioc->run();

	if(!recv_ec)
	{
		if(isNormalClose(recv_ec))
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		return recv_ec;
	}

	if(recv_bytes.size() <= 0)
		return ErrorCode{ERR_TIMEOUT};

	return recv_bytes;
}

ErrorCode MyWebsocketTLSClient::SendRaw(const byte* bytes, const size_t &len)
{
	boost::asio::const_buffer buffer(bytes, len);
	boost::beast::error_code ec;

	if(!ws.is_open())
		return {ERR_CONNECTION_CLOSED};
		
	size_t sendlen = ws.write(buffer, ec);
	if(ec)
		return {ec};
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};

	return {SUCCESS};
}

StackErrorCode MyWebsocketTLSClient::Connect(std::string addr, int port)
{
	Close();

	/*
	//sslctx에 인증서, 비밀키 로드
	sslctx.use_certificate_file(ConfigParser::GetString("CERT_FILE"), boost::asio::ssl::context::pem, ec);
	if(ec)
		return {ec, __STACKINFO__};
	sslctx.use_private_key_file(ConfigParser::GetString("KEY_FILE"), boost::asio::ssl::context::pem, ec);
	if(ec)
		return {ec, __STACKINFO__};
	*/

	boost::asio::ip::tcp::resolver resolver(*pioc);
	boost::beast::error_code ec;
	
	auto const results = resolver.resolve(addr, std::to_string(port), ec);
	if(ec)
		return {ec, __STACKINFO__};
	boost::asio::ip::tcp::socket socket(*pioc);
	for(auto &endpoint : results)
	{
		socket.connect(endpoint, ec);
	if(ec)
			continue;
	}
	if(ec)
		return {ec, __STACKINFO__};
	
	ws.next_layer() = boost::beast::ssl_stream<boost::beast::tcp_stream>(std::move(socket), sslctx);
	if(!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), addr.c_str()))
		return {ErrorCode(ERR_get_error()), __STACKINFO__};

	boost::beast::websocket::stream_base::timeout handshake_timeout;
	handshake_timeout.handshake_timeout = std::chrono::milliseconds((int)std::floor(TIME_HANDSHAKE * 1000));
	handshake_timeout.idle_timeout = boost::beast::websocket::stream_base::none();
	handshake_timeout.keep_alive_pings = false;
	ws.set_option(handshake_timeout);
	ws.next_layer().handshake(boost::asio::ssl::stream_base::client, ec);
	if(ec)
		return {ec, __STACKINFO__};

	ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-sync");
	}));

	ws.handshake(addr, "/", ec);
	if(ec)
		return {ec, __STACKINFO__};

	boost::asio::ip::tcp::endpoint ep = ws.next_layer().next_layer().socket().remote_endpoint();
	Address = ep.address().to_string();
	Port = ep.port();

	ws.binary(true);
	ws.next_layer().next_layer().socket().non_blocking(false);

	return {};
}

void MyWebsocketTLSClient::SetTimeout(float sec)
{
	int time = 0;
	if(sec > 0.0f)
	{
		long millisec = std::floor(sec * 1000);
		time = (int)millisec;
	}
	timeout = std::chrono::milliseconds(time);
}

void MyWebsocketTLSClient::Close()
{
	std::exception_ptr eptr = nullptr;
	deadline.cancel();
	if(ws.is_open())
	{
		ws.async_close(boost::beast::websocket::close_code::normal, [&](boost::beast::error_code err)
		{
			if(err == boost::asio::error::broken_pipe)	//SIGPIPE를 ignore해도 발생한다. Websocket은 Close Signal을 별도로 전송해야 되기 때문.
				return;
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
	isAsyncDone = true;
}