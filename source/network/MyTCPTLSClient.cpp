#include "MyTCPTLSClient.hpp"

MyTCPTLSClient::MyTCPTLSClient(boost::asio::ssl::context &sslctx_, boost::asio::ip::tcp::socket socket)
 : sslctx(boost::asio::ssl::context::tlsv12), ws(std::move(socket), sslctx_), MyClientSocket()
{
	timer = std::make_unique<boost::asio::steady_timer>(ws.get_executor());
	
	boost::system::error_code error_code;
	auto ep = ws.lowest_layer().remote_endpoint(error_code);
	if(error_code.failed())
	{
		Address = "";
		Port = 0;
		return;
	}
	
	Address = ep.address().to_string();
	Port = ep.port();
}

void MyTCPTLSClient::Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> callback)
{
	auto self_ptr = shared_from_this();
	ws.async_handshake(boost::asio::ssl::stream_base::server, [this, self_ptr, callback](boost::system::error_code error_code)
	{
		callback(self_ptr, ErrorCode(error_code));
	});
}

void MyTCPTLSClient::DoRecv(std::function<void(boost::system::error_code, size_t)> handler)
{
	ws.async_read_some(boost::asio::buffer(buffer.data(), BUF_SIZE), handler);
}

void MyTCPTLSClient::GetRecv(size_t bytes_written)
{
	bytes_written = std::min(bytes_written, BUF_SIZE);
	recvbuffer.Recv(buffer.data(), bytes_written);
}

void MyTCPTLSClient::DoSend(const byte* bytes, const size_t &len, std::function<void(boost::system::error_code, size_t)> handler)
{
	boost::asio::const_buffer send_buffer(bytes, len);
	ws.async_write_some(send_buffer, handler);
}

void MyTCPTLSClient::Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler, const boost::system::error_code& error_code)
{
	if(!error_code.failed())
	{
		/*
		//sslctx에 인증서, 비밀키 로드
		sslctx.use_certificate_file(ConfigParser::GetString("CERT_FILE", "/etc/ssl/drgb.crt"), boost::asio::ssl::context::pem, ec);
		if(ec)
			return {ec, __STACKINFO__};
		sslctx.use_private_key_file(ConfigParser::GetString("KEY_FILE", "/etc/ssl/drgb.key"), boost::asio::ssl::context::pem, ec);
		if(ec)
			return {ec, __STACKINFO__};
		*/
	
		boost::asio::ip::tcp::endpoint ep = ws.next_layer().remote_endpoint();
		Address = ep.address().to_string();
		Port = ep.port();

		auto self_ptr = shared_from_this();
		ws.async_handshake(boost::asio::ssl::stream_base::client, [this, self_ptr, handler](boost::system::error_code handshake_code)
		{
			if(handshake_code.failed())
				Connect_Handle(handler, handshake_code);
			else
				handler(self_ptr, ErrorCode(handshake_code));
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

		ws.next_layer().async_connect(ep, std::bind(&MyTCPTLSClient::Connect_Handle, this, handler, std::placeholders::_1));
	}
}

void MyTCPTLSClient::DoClose()
{
	auto self_ptr = shared_from_this();
	ws.async_shutdown([this, self_ptr](boost::system::error_code error_code)
	{
		if(error_code.failed())
		{

		}
		if(cleanHandler != nullptr)
			cleanHandler(self_ptr);
	});
	//boost::system::error_code error_code;
	//ws.next_layer().close(error_code);
}

boost::asio::any_io_executor MyTCPTLSClient::GetContext()
{
	return ws.get_executor();
}

bool MyTCPTLSClient::is_open() const
{
	return ws.next_layer().is_open();
}

bool MyTCPTLSClient::isReadable() const
{
	return is_open();
}