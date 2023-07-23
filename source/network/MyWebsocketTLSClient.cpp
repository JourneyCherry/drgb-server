#include "MyWebsocketTLSClient.hpp"

MyWebsocketTLSClient::MyWebsocketTLSClient(boost::asio::ssl::context &sslctx_, boost::asio::ip::tcp::socket socket)
 : sslctx(boost::asio::ssl::context::tlsv12), ws(std::move(socket), sslctx_), MyClientSocket()
{
	timer = std::make_unique<boost::asio::steady_timer>(ws.get_executor());

	boost::beast::error_code ec;
	boost::asio::ip::tcp::endpoint ep = boost::beast::get_lowest_layer(ws).socket().remote_endpoint(ec);
	if(ec.failed())
	{
		Address = "";
		Port = 0;
		return;
	}
	
	Address = ep.address().to_string();
	Port = ep.port();

	//boost::beast::websocket::stream_base::timeout handshake_timeout;
	//handshake_timeout.handshake_timeout = std::chrono::milliseconds(TIME_HANDSHAKE);
	//handshake_timeout.idle_timeout = boost::beast::websocket::stream_base::none();
	//handshake_timeout.keep_alive_pings = false;
	//ws.set_option(handshake_timeout);
	ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
}

void MyWebsocketTLSClient::Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> callback)
{
	auto self_ptr = shared_from_this();
	ws.next_layer().async_handshake(boost::asio::ssl::stream_base::server, [this, self_ptr, callback](boost::system::error_code handshake_code)
	{
		if(handshake_code.failed())
		{
			callback(self_ptr, ErrorCode(handshake_code));
		}
		else
		{
			ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
				res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
			}));

			ws.async_accept([this, self_ptr, callback](boost::system::error_code accept_code)
			{
				if(!accept_code.failed())
				{
					this->ws.binary(true);
					this->ws.next_layer().next_layer().socket().non_blocking(false);
				}
				callback(self_ptr, ErrorCode(accept_code));
			});
		}
	});
}

void MyWebsocketTLSClient::DoRecv(std::function<void(boost::system::error_code, size_t)> handler)
{
	ws.async_read(buffer, handler);
}

void MyWebsocketTLSClient::GetRecv(size_t bytes_written)
{
	if(bytes_written > 0)
	{
		if(!ws.got_binary())
			throw ErrorCodeExcept(ERR_PROTOCOL_VIOLATION, __STACKINFO__);

		unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
		size_t size = boost::asio::buffer_size(buffer.data());

		recvbuffer.Recv(first, size);

		buffer.consume(bytes_written);
		//buffer.clear();
	}
}

void MyWebsocketTLSClient::DoSend(const byte* bytes, const size_t &len, std::function<void(boost::system::error_code, size_t)> handler)
{
	boost::asio::const_buffer send_buffer(bytes, len);
	ws.async_write(send_buffer, handler);
}

void MyWebsocketTLSClient::Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler, const boost::system::error_code &error_code)
{
	if(!error_code.failed())
	{
		DomainStr = Address + ":" + std::to_string(Port);

		boost::asio::ip::tcp::endpoint ep = ws.next_layer().next_layer().socket().remote_endpoint();
		Address = ep.address().to_string();
		Port = ep.port();

		/*
		//sslctx에 인증서, 비밀키 로드
		sslctx.use_certificate_file(ConfigParser::GetString("CERT_FILE", "/etc/ssl/drgb.crt"), boost::asio::ssl::context::pem, ec);
		if(ec)
			return {ec, __STACKINFO__};
		sslctx.use_private_key_file(ConfigParser::GetString("KEY_FILE", "/etc/ssl/drgb.key"), boost::asio::ssl::context::pem, ec);
		if(ec)
			return {ec, __STACKINFO__};
		*/

		if(!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), DomainStr.c_str()))
			throw ErrorCodeExcept{ErrorCode(ERR_get_error()), __STACKINFO__};

		//boost::beast::websocket::stream_base::timeout handshake_timeout;
		//handshake_timeout.handshake_timeout = std::chrono::milliseconds(TIME_HANDSHAKE);
		//handshake_timeout.idle_timeout = boost::beast::websocket::stream_base::none();
		//handshake_timeout.keep_alive_pings = false;
		//ws.set_option(handshake_timeout);
		ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

		auto self_ptr = shared_from_this();
		ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client, [this, self_ptr, handler](boost::system::error_code ssl_handshake_code)
		{
			if(ssl_handshake_code.failed())
			{
				this->Connect_Handle(handler, ssl_handshake_code);
			}
			else
			{
				ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
					res.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-sync");
				}));

				ws.async_handshake(DomainStr, "/", [this, self_ptr, handler](boost::system::error_code handshake_code)
				{
					if(handshake_code.failed())
					{
						this->Connect_Handle(handler, handshake_code);
					}
					else
					{
						this->ws.binary(true);
						this->ws.next_layer().next_layer().socket().non_blocking(false);
						handler(self_ptr, ErrorCode(handshake_code));
					}
				});

			}
		});

	}
	else
	{
		if(endpoints.empty())
		{
			handler(shared_from_this(), ErrorCode(error_code));
			return;
		}
		auto ep = endpoints.front();
		endpoints.pop();

		ws.next_layer().next_layer().async_connect(ep, std::bind(&MyWebsocketTLSClient::Connect_Handle, this, handler, std::placeholders::_1));
	}
}

void MyWebsocketTLSClient::DoClose()
{
	//boost::system::error_code error_code;
	//ws.close(boost::beast::websocket::close_code::normal, error_code);
	auto self_ptr = shared_from_this();
	ws.async_close(boost::beast::websocket::close_code::normal, [this, self_ptr](boost::system::error_code error_code)
	{
		if(error_code.failed())
		{

		}
		if(cleanHandler != nullptr)
			cleanHandler(self_ptr);
	});
}

boost::asio::any_io_executor MyWebsocketTLSClient::GetContext()
{
	return ws.get_executor();
}

bool MyWebsocketTLSClient::is_open() const
{
	return ws.next_layer().next_layer().socket().is_open();
}

bool MyWebsocketTLSClient::isReadable() const
{
	return ws.is_open();
}