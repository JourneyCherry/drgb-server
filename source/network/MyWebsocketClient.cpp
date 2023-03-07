#include "MyWebsocketClient.hpp"

MyWebsocketClient::MyWebsocketClient(boost::asio::ip::tcp::socket socket) : ws{std::move(socket)}, MyClientSocket()
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

	boost::beast::error_code ec;
	boost::beast::flat_buffer buffer;
	while(!recvbuffer.isMsgIn())
	{
		buffer.clear();
		if(!ws.is_open())	//읽는 도중에 종료될 경우.
			return {ErrorCode{ERR_CONNECTION_CLOSED}};

		size_t recvlen = ws.read(buffer, ec);	//ws->read()는 buffer에 새 데이터를 append 한다.
		if(recvlen == 0)
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		if(
			ec == boost::beast::websocket::error::closed || 
			ec == boost::asio::error::eof ||
			ec == boost::asio::error::operation_aborted
		)
			return {ErrorCode{ERR_CONNECTION_CLOSED}};
		if(ec)
			return {ErrorCode{ec}};

		if(!ws.got_binary())
			return {ErrorCode{ERR_PROTOCOL_VIOLATION}};

		unsigned char *first = boost::asio::buffer_cast<unsigned char*>(buffer.data());
		size_t size = boost::asio::buffer_size(buffer.data());

		recvbuffer.Recv(first, size);
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
		while(!ws.is_message_done()){}	//메시지를 읽는 도중에 종료하지 않는다.
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
}