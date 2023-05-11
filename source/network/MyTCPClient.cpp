#include "MyTCPClient.hpp"

MyTCPClient::MyTCPClient() : isAsyncDone(true), timeout(0), pioc(std::make_shared<boost::asio::io_context>()), socket(*pioc), deadline(*pioc), MyClientSocket() {}

MyTCPClient::MyTCPClient(std::shared_ptr<boost::asio::io_context> _pioc, boost::asio::ip::tcp::socket sock)
 : isAsyncDone(true), timeout(0), pioc(_pioc), socket(std::move(sock)), deadline(*pioc), MyClientSocket()
{
	boost::asio::ip::tcp::endpoint ep = socket.remote_endpoint();
	Address = ep.address().to_string();
	Port = ep.port();

	socket.non_blocking(false);
}

MyTCPClient::~MyTCPClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyTCPClient::RecvRaw()
{
	if(!socket.is_open())
		return {ErrorCode{ERR_CONNECTION_CLOSED}};

	if(isAsyncDone)
	{
		isAsyncDone = false;
		recv_ec = ErrorCode(SUCCESS);
		result_bytes.clear();
		if(timeout.count() > 0)
		{
			deadline.expires_after(timeout);
			deadline.async_wait([&](boost::beast::error_code error)
			{
				if(error == boost::asio::error::operation_aborted)
					return;
				boost::system::error_code cancel_ec;
				socket.cancel(cancel_ec);
				if(cancel_ec)
					recv_ec = ErrorCode(cancel_ec);
			});
		}
		socket.async_receive(boost::asio::buffer(buffer.data(), BUF_SIZE), [&](boost::beast::error_code error, size_t bytes_written)
		{
			isAsyncDone = true;
			if(error == boost::asio::error::operation_aborted)
				recv_ec = ErrorCode(ERR_TIMEOUT);
			else
				recv_ec = ErrorCode(error);
			if(!recv_ec)
				return;
			if(bytes_written == 0)
			{
				recv_ec = ErrorCode(ERR_CONNECTION_CLOSED);
				return;
			}

			unsigned char *first = static_cast<unsigned char*>(buffer.data());
			size_t size = bytes_written;

			result_bytes.assign(first, first + size);
			//buffer = boost::asio::mutable_buffer();

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
	if(result_bytes.size() <= 0)
		return ErrorCode{ERR_TIMEOUT};

	return result_bytes;
}

ErrorCode MyTCPClient::SendRaw(const byte* bytes, const size_t &len)
{
	if(!socket.is_open())
		return {ERR_CONNECTION_CLOSED};

	boost::asio::const_buffer buffer(bytes, len);
	boost::beast::error_code ec;
	
	size_t sendlen = socket.send(buffer, 0, ec);
	if(ec)
		return {ec};
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};

	return {SUCCESS};
}

StackErrorCode MyTCPClient::Connect(std::string addr, int port)
{
	Close();

	boost::asio::ip::tcp::resolver resolver(*pioc);
	boost::beast::error_code ec;

	auto const results = resolver.resolve(addr, std::to_string(port), ec);
	if(ec)
		return {ec, __STACKINFO__};
	for(auto &endpoint : results)
	{
		socket.connect(endpoint, ec);
		if(ec)
			continue;
		break;
	}
	if(ec)
		return {ec, __STACKINFO__};

	boost::asio::ip::tcp::endpoint ep = socket.remote_endpoint();
	Address = ep.address().to_string();
	Port = ep.port();

	socket.non_blocking(false);
	
	return {};
}

void MyTCPClient::SetTimeout(float sec)
{
	int time = 0;
	if(sec > 0.0f)
	{
		long millisec = std::floor(sec * 1000);
		time = (int)millisec;
	}
	timeout = std::chrono::milliseconds(time);
}

void MyTCPClient::Close()
{
	std::exception_ptr eptr = nullptr;
	boost::system::error_code ec;

	if(socket.is_open())
		socket.close(ec);
	deadline.cancel(ec);
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
	if(ec)
		throw ErrorCodeExcept(ec, __STACKINFO__);
	isAsyncDone = true;
}