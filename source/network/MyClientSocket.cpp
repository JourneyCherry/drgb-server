#include "MyClientSocket.hpp"

MyClientSocket::MyClientSocket() : cleanHandler(nullptr), encryptor(true), decryptor(false), isSecure(false), initiateClose(false)
{
}

void MyClientSocket::KeyExchange(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler)
{
	auto pkey = exchanger.GetPublicKey();
	if(!pkey)
	{
		handler(shared_from_this(), pkey.error());
		return;
	}
	Send(*pkey);

	if(!isReadable())
	{
		handler(shared_from_this(), ErrorCode(ERR_CONNECTION_CLOSED));
		return;
	}
	
	SetTimeout(TIME_KEYEXCHANGE, [this, handler](std::shared_ptr<MyClientSocket> socket){
		Close();
	});

	StartRecv([this, handler](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		CancelTimeout();
		if(!recv_ec)
		{
			handler(client, recv_ec);
			return;
		}

		auto secret = exchanger.ComputeKey(packet.vector());
		if(!secret)
		{
			handler(client, ErrorCode(ERR_PROTOCOL_VIOLATION));
			return;
		}
		auto derived = KeyExchanger::KDF(*secret, 48);
		if(derived.size() != 48)
		{
			handler(client, ErrorCode(ERR_OUT_OF_CAPACITY));
			return;
		}
		
		auto [key, iv] = Encryptor::SplitKey(derived);

		encryptor.SetKey(key, iv);
		decryptor.SetKey(key, iv);

		isSecure = true;
		handler(client, ErrorCode(SUCCESS));
	});
}

Expected<ByteQueue> MyClientSocket::Recv()
{
	if(recvbuffer.isMsgIn())
	{
		auto result = recvbuffer.GetMsg();
		if(isSecure)
			result = decryptor.Crypt(result);
		return ByteQueue(result);
	}
	return {};
}

void MyClientSocket::StartRecv(std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handler)
{
	std::unique_lock<std::mutex> lk(mtx);
	if(!isReadable() || initiateClose)
		return;
	lk.unlock();

	auto msg = Recv();
	if(msg)
		handler(shared_from_this(), *msg, ErrorCode(SUCCESS));
	else
		DoRecv(std::bind(&MyClientSocket::Recv_Handle, this, shared_from_this(), std::placeholders::_1, std::placeholders::_2, handler));
}

void MyClientSocket::Recv_Handle(std::shared_ptr<MyClientSocket> self_ptr, boost::system::error_code error_code, size_t bytes_written, std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handler)
{
	if(self_ptr == nullptr)
		return;

	if(error_code.failed())
	{
		handler(self_ptr, ByteQueue{}, ErrorCode{error_code});
		self_ptr->Close();
		return;
	}

	self_ptr->GetRecv(bytes_written);
	
	auto result = Recv();
	if(result)
		handler(self_ptr, *result, ErrorCode(SUCCESS));
	else
		StartRecv(handler);
}

void MyClientSocket::Send(ByteQueue bytes)
{
	return Send(bytes.vector());
}

void MyClientSocket::Send(std::vector<byte> bytes)
{
	std::unique_lock<std::mutex> lk(mtx);
	if(!isReadable() || initiateClose)
		return;
	lk.unlock();

	if(isSecure)
		bytes = PacketProcessor::encapsulate(encryptor.Crypt(bytes));
	else
		bytes = PacketProcessor::encapsulate(bytes);

	std::unique_lock<std::mutex> send_lk(send_mtx);
	send_queue.push(bytes);
	if(send_queue.size() > 1)	//첫번째 element는 send 중인 element이다.
		return;
	DoSend(bytes.data(), bytes.size(), std::bind(&MyClientSocket::Send_Handle, this, shared_from_this(), std::placeholders::_1, std::placeholders::_2, bytes));
}

void MyClientSocket::Send_Handle(std::shared_ptr<MyClientSocket> client, boost::system::error_code error_code, size_t bytes_sent, std::vector<byte> bytes)
{
	if(error_code.failed() || bytes_sent <= 0)
	{
		client->Close();
		return;
	}

	if(!isReadable())
		return;
	else if(bytes_sent < bytes.size())
	{
		bytes.erase(bytes.begin(), bytes.begin() + bytes_sent);
		client->DoSend(bytes.data(), bytes.size(), std::bind(&MyClientSocket::Send_Handle, this, client, std::placeholders::_1, std::placeholders::_2, bytes));
	}
	else
	{
		std::unique_lock<std::mutex> lk(send_mtx);
		send_queue.pop();
		if(!send_queue.empty())
		{
			auto next_bytes = send_queue.front();
			client->DoSend(next_bytes.data(), next_bytes.size(), std::bind(&MyClientSocket::Send_Handle, this, client, std::placeholders::_1, std::placeholders::_2, next_bytes));
		}
	}
}

void MyClientSocket::Connect(std::string addr, int port, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler)
{
	if(is_open())
		return;
	
	while(!endpoints.empty())
		endpoints.pop();

	//For Websocket
	Address = addr;
	Port = port;

	boost::asio::ip::tcp::resolver resolver(GetContext());
	boost::beast::error_code ec;

	auto results = resolver.resolve(addr, std::to_string(port), ec);
	if(ec)
		throw ErrorCodeExcept{ec, __STACKINFO__};
	for(auto &endpoint : results)
		endpoints.push(endpoint);

	Connect_Handle(handler, boost::asio::error::connection_refused);
}

void MyClientSocket::SetTimeout(const int& ms, std::function<void(std::shared_ptr<MyClientSocket>)> callback)
{
	if(initiateClose)
		return;
	timer->expires_after(std::chrono::milliseconds(ms));
	auto self_ptr = shared_from_this();
	timer->async_wait([this, self_ptr, callback](const boost::system::error_code& error_code)
	{
		if(error_code != boost::asio::error::operation_aborted)
			callback(self_ptr);
	});
}

void MyClientSocket::SetCleanUp(std::function<void(std::shared_ptr<MyClientSocket>)> callback)
{
	cleanHandler = callback;
}

void MyClientSocket::CancelTimeout()
{
	timer->cancel();
}

void MyClientSocket::Close()
{
	std::unique_lock<std::mutex> lk(mtx);
	if(initiateClose)
		return;

	initiateClose = true;
	lk.unlock();

	CancelTimeout();
	if(isReadable())
		DoClose();
	else
	{
		if(cleanHandler != nullptr)
			cleanHandler(shared_from_this());
	}
}

void MyClientSocket::FillTTL(const int& max_ttl)
{
	ttl = max_ttl;
}

bool MyClientSocket::CountTTL()
{
	ttl -= 1;
	return ttl > 0;
}

bool MyClientSocket::isNormalClose(const ErrorCode &ec)
{
	if(ec)
		return true;
	
	switch(ec.typecode())
	{
		case ErrorCode::TYPE_ERRNO:
			return (ec.code() == EINVAL);
		case ErrorCode::TYPE_CUSTOM:
			return (ec.code() == ERR_CONNECTION_CLOSED);
		case ErrorCode::TYPE_BOOST:
			switch(ec.code())
			{
				case (int)boost::beast::websocket::error::closed:	//it is enum class
				case boost::asio::error::eof:
				case boost::asio::error::operation_aborted:
					return true;
			}
			return false;
	}
	return false;
}

std::string MyClientSocket::ToString()
{
	return Address + ":" + std::to_string(Port);
}

std::string MyClientSocket::GetAddress()
{
	return Address;
}

int MyClientSocket::GetPort()
{
	return Port;
}