#include "MyClientSocket.hpp"

MyClientSocket::MyClientSocket() : encryptor(true), decryptor(false), isSecure(false), initiateClose(false), initiateRecv(false)
{
}

void MyClientSocket::KeyExchange(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler)
{
	keHandler = handler;
	auto pkey = exchanger.GetPublicKey();
	if(!pkey)
	{
		keHandler(shared_from_this(), pkey.error());
		return;
	}
	ErrorCode ec = Send(*pkey);
	if(!ec)
	{
		keHandler(shared_from_this(), ec);
		return;
	}
	
	SetTimeout(TIME_KEYEXCHANGE, [this](std::shared_ptr<MyClientSocket> socket){
		this->keHandler(socket, ErrorCode(ERR_TIMEOUT));
	});
	
	std::unique_lock<std::mutex> lk(mtx);
	if(isReadable())
		DoRecv(std::bind(&MyClientSocket::KE_Handle, this, std::placeholders::_1, std::placeholders::_2));
	else
	{
		CancelTimeout();
		keHandler(shared_from_this(), ErrorCode(ERR_CONNECTION_CLOSED));
	}
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

void MyClientSocket::KE_Handle(boost::system::error_code error_code, size_t bytes_written)
{
	if(error_code.failed())
	{
		keHandler(shared_from_this(), ErrorCode(error_code));
		return;
	}
	else
		GetRecv(bytes_written);

	auto result = Recv();
	if(!result)
	{
		DoRecv(std::bind(&MyClientSocket::KE_Handle, this, std::placeholders::_1, std::placeholders::_2));
		return;
	}
	CancelTimeout();

	auto secret = exchanger.ComputeKey(result->vector());
	if(!secret)
	{
		keHandler(shared_from_this(), ErrorCode(ERR_PROTOCOL_VIOLATION));
		return;
	}
	auto derived = KeyExchanger::KDF(*secret, 48);
	if(derived.size() != 48)
	{
		keHandler(shared_from_this(), ErrorCode(ERR_OUT_OF_CAPACITY));
		return;
	}
	
	auto [key, iv] = Encryptor::SplitKey(derived);

	encryptor.SetKey(key, iv);
	decryptor.SetKey(key, iv);

	isSecure = true;
	keHandler(shared_from_this(), ErrorCode(SUCCESS));
}

void MyClientSocket::StartRecv(std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> callback)
{
	msgHandler = callback;

	std::unique_lock<std::mutex> lk(mtx);
	if(!isReadable() || initiateClose)
		return;

	if(!initiateRecv)
	{
		initiateRecv = true;
		DoRecv(std::bind(&MyClientSocket::Recv_Handle, this, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}
}

void MyClientSocket::Recv_Handle(std::shared_ptr<MyClientSocket> self_ptr, boost::system::error_code error_code, size_t bytes_written)
{
	if(self_ptr == nullptr)
		return;

	self_ptr->GetRecv(bytes_written);
	
	while(msgHandler != nullptr && recvbuffer.isMsgIn())
	{
		auto result = Recv();
		if(!result)
			break;
		msgHandler(shared_from_this(), *result, ErrorCode(SUCCESS));
	}

	if(error_code.failed())
	{
		if(msgHandler != nullptr)
			msgHandler(shared_from_this(), ByteQueue{}, ErrorCode{error_code});
		self_ptr->Close();
		initiateRecv = false;
		return;
	}

	std::unique_lock<std::mutex> lk(mtx);
	if(!isReadable() || initiateClose)
		return;

	DoRecv(std::bind(&MyClientSocket::Recv_Handle, this, shared_from_this(), std::placeholders::_1, std::placeholders::_2));	//읽을 수 있으면 계속 읽는다.
}

ErrorCode MyClientSocket::Send(ByteQueue bytes)
{
	return Send(bytes.vector());
}

ErrorCode MyClientSocket::Send(std::vector<byte> bytes)
{
	std::unique_lock<std::mutex> lk(mtx);
	if(!is_open() || initiateClose)
		return ErrorCode(ERR_CONNECTION_CLOSED);

	if(isSecure)
		bytes = PacketProcessor::encapsulate(encryptor.Crypt(bytes));
	else
		bytes = PacketProcessor::encapsulate(bytes);

	auto result = DoSend(bytes.data(), bytes.size());

	return result;
}

void MyClientSocket::Connect(std::string addr, int port, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handler)
{
	if(is_open())
		return;
	
	while(!endpoints.empty())
		endpoints.pop();

	connHandler = handler;

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

	Connect_Handle(boost::asio::error::connection_refused);
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
		{
			if(callback == nullptr)
				this->msgHandler(self_ptr, ByteQueue(), ErrorCode(ERR_TIMEOUT));
			else
				callback(self_ptr);
		}
	});
}

void MyClientSocket::SetCleanUp(std::function<void(std::shared_ptr<MyClientSocket>)> callback)
{
	cleanHandler = callback;
}

void MyClientSocket::CleanUp()
{
	if(cleanHandler == nullptr)
		return;
	boost::asio::post(ioc_ref, std::bind(cleanHandler, shared_from_this()));
}

void MyClientSocket::CancelTimeout()
{
	timer->cancel();
}

void MyClientSocket::Close()
{
	std::unique_lock<std::mutex> lk(mtx);
	if(!is_open())
		return;
	
	if(initiateClose)
		return;

	initiateClose = true;
	CancelTimeout();
	Cancel();
	Shutdown();
	lk.unlock();

	CleanUp();
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