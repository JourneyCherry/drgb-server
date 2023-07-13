#include "MyWebsocketClient.hpp"

MyWebsocketClient::MyWebsocketClient(boost::asio::ip::tcp::socket socket_) : ws(std::move(socket_)), MyClientSocket()
{
	ioc_ref = ws.get_executor();
	timer = std::make_unique<timer_t>(ioc_ref);
	
	boost::system::error_code error_code;
	boost::asio::ip::tcp::endpoint ep = ws.next_layer().socket().remote_endpoint(error_code);
	if(error_code.failed())
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

	ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
	}));
}

MyWebsocketClient::~MyWebsocketClient()
{
	Shutdown();
}

void MyWebsocketClient::Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> callback)
{
	auto self_ptr = shared_from_this();
	ws.async_accept([this, self_ptr, callback](boost::system::error_code accept_code)
	{
		if(!accept_code.failed())
		{
			ws.binary(true);
			ws.next_layer().socket().non_blocking(false);
		}
		callback(self_ptr, ErrorCode(accept_code));
	});
}

void MyWebsocketClient::DoRecv(std::function<void(boost::system::error_code, size_t)> handler)
{
	ws.async_read(buffer, handler);
}

void MyWebsocketClient::GetRecv(size_t bytes_written)
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

ErrorCode MyWebsocketClient::DoSend(const byte* bytes, const size_t &len)
{
	boost::asio::const_buffer send_buffer(bytes, len);

	boost::system::error_code ec;
	ws.write(send_buffer, ec);
	return ErrorCode(ec);

	//ws.async_write(send_buffer, std::bind(&MyWebsocketClient::Send_Handle, this, std::placeholders::_1, std::placeholders::_2));
}

void MyWebsocketClient::Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler, const boost::system::error_code &error_code)
{
	if(!error_code.failed())
	{
		DomainStr = Address + ":" + std::to_string(Port);
		boost::asio::ip::tcp::endpoint ep = ws.next_layer().socket().remote_endpoint();
		Address = ep.address().to_string();
		Port = ep.port();

		//boost::beast::websocket::stream_base::timeout handshake_timeout;
		//handshake_timeout.handshake_timeout = std::chrono::milliseconds(TIME_HANDSHAKE);
		//handshake_timeout.idle_timeout = boost::beast::websocket::stream_base::none();
		//handshake_timeout.keep_alive_pings = false;
		//ws.set_option(handshake_timeout);
		ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

		ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
			res.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
		}));

		auto self_ptr = shared_from_this();
		ws.async_handshake(DomainStr, "/", [this, self_ptr, handler](boost::system::error_code handshake_code)
		{
			if(handshake_code.failed())
			{
				this->Connect_Handle(handler, handshake_code);
			}
			else
			{
				this->ws.binary(true);
				this->ws.next_layer().socket().non_blocking(false);
				handler(self_ptr, ErrorCode(handshake_code));
			}
		});

	}
	else
	{
		if(endpoints.empty())
		{
			handler(shared_from_this(), ErrorCode(ERR_CONNECTION_CLOSED));
			return;
		}
		auto ep = endpoints.front();
		endpoints.pop();

		ws.next_layer().async_connect(ep, std::bind(&MyWebsocketClient::Connect_Handle, this, handler, std::placeholders::_1));
	}
}

void MyWebsocketClient::Cancel()
{
	boost::system::error_code error_code;
	ws.next_layer().socket().cancel(error_code);
}

void MyWebsocketClient::Shutdown()
{
	if(isReadable())
	{
		//boost::system::error_code error_code;
		//ws.close(boost::beast::websocket::close_code::normal, error_code);
		auto self_ptr = shared_from_this();
		ws.async_close(boost::beast::websocket::close_code::normal, [this, self_ptr](boost::system::error_code error_code)
		{
			if(self_ptr->is_open())
				this->ws.next_layer().socket().close(error_code);
		});
	}
}

boost::asio::any_io_executor MyWebsocketClient::GetContext()
{
	return ws.get_executor();
}

bool MyWebsocketClient::is_open() const
{
	return ws.next_layer().socket().is_open();
}

bool MyWebsocketClient::isReadable() const
{
	return ws.is_open();
}