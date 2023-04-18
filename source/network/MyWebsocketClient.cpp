#include "MyWebsocketClient.hpp"

MyWebsocketClient::MyWebsocketClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ip::tcp::socket socket)
 : pioc(_pioc), ws(nullptr), isAsyncDone(true), MyClientSocket()
{
	ws = std::make_unique<booststream>(std::move(socket));
	auto &sock = ws->next_layer();
	boost::asio::ip::tcp::endpoint ep = sock.remote_endpoint();
	Address = ep.address().to_string() + ":" + std::to_string(ep.port());

	ws->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
	}));

	boost::beast::error_code ec;

	SetTimeout(TIME_HANDSHAKE);
	ws->accept(ec);
	SetTimeout();

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
	pioc->run_one();	//set_option(timeout)으로 인해 deadline_timer가 io_context에 같이 붙게 되므로, 가장 먼저 호출되는 handler만 처리하고 blocking에서 빠져나와야 한다.

	if(!ec)
	{
		if(isNormalClose(ec))
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		return ec;
	}

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

	pioc = std::make_shared<boost::asio::io_context>();
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

	ws = std::make_unique<booststream>(std::move(socket));
	SetTimeout(TIME_HANDSHAKE);
	ws->handshake(addr, "/", ec);
	SetTimeout();
	if(ec)
		return {ec, __STACKINFO__};

	boost::asio::ip::tcp::endpoint ep = socket.remote_endpoint();
	Address = ep.address().to_string() + ":" + std::to_string(ep.port());

	ws->binary(true);
	
	return {};
}

void MyWebsocketClient::SetTimeout(float sec)
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
	opt.keep_alive_pings = false;
	ws->set_option(opt);
}

void MyWebsocketClient::Close()
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