#include "MyWebsocketClient.hpp"

MyWebsocketClient::MyWebsocketClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ip::tcp::socket socket)
 : pioc(_pioc), ws{std::move(socket)}, MyClientSocket()
{
	auto &sock = ws.next_layer();
	boost::asio::ip::tcp::endpoint ep = sock.remote_endpoint();
	Address = ep.address().to_string() + ":" + std::to_string(ep.port());

	ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res){
		res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
	}));

	boost::beast::error_code ec;
	ws.accept(ec);
	if(ec)
	{
		Close();
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	ws.binary(true);
}

MyWebsocketClient::~MyWebsocketClient()
{
	Close();
}

Expected<ByteQueue, ErrorCode> MyWebsocketClient::Recv()
{
	if(!ws.is_open())
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	while(!recvbuffer.isMsgIn())
	{
		if(!ws.is_open())	//읽는 도중에 종료될 경우.
			return {ErrorCode{ERR_CONNECTION_CLOSED}};

		boost::beast::error_code result;
		boost::beast::flat_buffer buffer;

		ws.async_read(buffer, [&](boost::beast::error_code ec, size_t bytes_written)
		{
			result = ec;
			if(ec)
				return;
			if(bytes_written == 0)
				return;
			if(!ws.got_binary())
				return;

			unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
			size_t size = boost::asio::buffer_size(buffer.data());

			recvbuffer.Recv(first, size);
		});

		if(pioc->stopped())
			pioc->restart();
		pioc->run();

		if(!ws.got_binary())
			return {ErrorCode{ERR_PROTOCOL_VIOLATION}};
		if(
			result == boost::beast::websocket::error::closed || 
			result == boost::asio::error::eof ||
			result == boost::asio::error::operation_aborted
		)
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		if(result)
			return {ErrorCode{result}};
	}

	return recvbuffer.GetMsg();
}

ErrorCode MyWebsocketClient::Send(ByteQueue bytes)
{
	ByteQueue capsulated = PacketProcessor::enpackage(bytes);
	boost::asio::const_buffer buffer(capsulated.data(), capsulated.Size());
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

void MyWebsocketClient::Close()
{
	if(ws.is_open())
	{
		boost::beast::error_code ec;
		ws.close(boost::beast::websocket::close_code::normal, ec);
		if(
			ec == boost::beast::websocket::error::closed || 
			ec == boost::asio::error::eof ||
			ec == boost::asio::error::operation_aborted
		)
			return;
		if(ec)
			throw ErrorCodeExcept(ec, __STACKINFO__);
	}
	pioc->stop();
}