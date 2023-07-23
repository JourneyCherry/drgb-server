#include "MyTCPClient.hpp"

MyTCPClient::MyTCPClient(boost::asio::ip::tcp::socket _socket) : socket(std::move(_socket)), MyClientSocket()
{
	timer = std::make_unique<boost::asio::steady_timer>(socket.get_executor());

	boost::system::error_code error_code;
	boost::asio::ip::tcp::endpoint ep = socket.remote_endpoint(error_code);
	if(error_code.failed())
	{
		Address = "";
		Port = 0;
		return;
	}

	Address = ep.address().to_string();
	Port = ep.port();

	socket.non_blocking(false);
}

void MyTCPClient::Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> callback)
{
	auto self_ptr = shared_from_this();
	socket.async_wait(boost::asio::ip::tcp::socket::wait_read, [this, self_ptr, callback](boost::system::error_code error_code)
	{
		if(error_code.failed())
		{
			callback(self_ptr, ErrorCode(error_code));
		}
		else
			callback(self_ptr, ErrorCode(SUCCESS));
	});
}

void MyTCPClient::DoRecv(std::function<void(boost::system::error_code, size_t)> handler)
{
	socket.async_receive(boost::asio::buffer(buffer.data(), BUF_SIZE), handler);
}

void MyTCPClient::GetRecv(size_t bytes_written)
{
	bytes_written = std::min(bytes_written, BUF_SIZE);
	recvbuffer.Recv(buffer.data(), bytes_written);
}

void MyTCPClient::DoSend(const byte* bytes, const size_t &len, std::function<void(boost::system::error_code, size_t)> handler)
{
	boost::asio::const_buffer send_buffer(bytes, len);
	socket.async_send(send_buffer, handler);
}

void MyTCPClient::Connect_Handle(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler, const boost::system::error_code& error_code)
{
	if(!error_code.failed())
	{
		boost::asio::ip::tcp::endpoint ep = socket.remote_endpoint();
		Address = ep.address().to_string();
		Port = ep.port();

		socket.non_blocking(false);
		
		handler(shared_from_this(), ErrorCode(error_code));
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
		
		socket.async_connect(ep, std::bind(&MyTCPClient::Connect_Handle, this, handler, std::placeholders::_1));
	}
}

void MyTCPClient::DoClose()
{
	socket.async_wait(boost::asio::ip::tcp::socket::wait_write, [this](boost::system::error_code error_code)
	{
		socket.close(error_code);
		if(cleanHandler != nullptr)
			cleanHandler(shared_from_this());
	});
}

boost::asio::any_io_executor MyTCPClient::GetContext()
{
	return socket.get_executor();
}

bool MyTCPClient::is_open() const
{
	return socket.is_open();
}

bool MyTCPClient::isReadable() const
{
	return socket.is_open();
}