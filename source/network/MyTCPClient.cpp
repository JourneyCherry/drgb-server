#include "MyTCPClient.hpp"

MyTCPClient::MyTCPClient(boost::asio::ip::tcp::socket _socket) : socket(std::move(_socket)), MyClientSocket()
{
	ioc_ref = socket.get_executor();
	timer = std::make_unique<timer_t>(ioc_ref);

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

MyTCPClient::~MyTCPClient()
{
	Shutdown();
}

void MyTCPClient::Prepare(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> callback)
{
	auto self_ptr = shared_from_this();
	boost::asio::post(ioc_ref, [this, self_ptr, callback]()
	{
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

ErrorCode MyTCPClient::DoSend(const byte* bytes, const size_t &len)
{
	boost::asio::const_buffer send_buffer(bytes, len);
	
	boost::system::error_code ec;
	socket.send(send_buffer, 0, ec);
	return ErrorCode{ec};

	//socket.async_send(send_buffer, std::bind(&MyTCPClient::Send_Handle, this, std::placeholders::_1, std::placeholders::_2));
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

void MyTCPClient::Cancel()
{
	boost::system::error_code error_code;
	socket.cancel(error_code);
	//ErrorCode ec(error_code);
	//if(!isNormalClose(ec))
	//	throw ErrorCodeExcept(ec, __STACKINFO__);
}

void MyTCPClient::Shutdown()
{
	boost::system::error_code error_code;
	socket.close(error_code);
	//ErrorCode ec(error_code);
	//if(!isNormalClose(ec))
	//	throw ErrorCodeExcept(ec, __STACKINFO__);
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