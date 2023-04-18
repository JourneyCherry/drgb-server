#include "MyClientSocket.hpp"

MyClientSocket::MyClientSocket() : encryptor(true), decryptor(false), isSecure(false)
{
}

StackErrorCode MyClientSocket::KeyExchange(bool useSecure)
{
	if(!useSecure)
		return {};

	ErrorCode ec;
	KeyExchanger exchanger;
	isSecure = false;


	auto pkey = exchanger.GetPublicKey();
	if(!pkey)
		return pkey.error();
	ec = Send(*pkey);
	if(!ec)
		return {ec, __STACKINFO__};

	auto peer_key = Recv(TIME_KEYEXCHANGE);
	
	if(!peer_key)
		return {peer_key.error(), __STACKINFO__};
	auto secret = exchanger.ComputeKey(peer_key->vector());
	if(!secret)
		return {secret.error(), __STACKINFO__};
	auto derived = KeyExchanger::KDF(*secret, 48);
	if(derived.size() != 48)
		return {ErrorCode(ERR_PROTOCOL_VIOLATION), __STACKINFO__};
	
	auto [key, iv] = Encryptor::SplitKey(derived);

	encryptor.SetKey(key, iv);
	decryptor.SetKey(key, iv);

	isSecure = true;

	return {};
}

std::string MyClientSocket::ToString()
{
	return Address;
}

Expected<ByteQueue, ErrorCode> MyClientSocket::Recv(float timeout)
{
	if(timeout > 0.0f)
		SetTimeout(timeout);

	while(!recvbuffer.isMsgIn())
	{
		auto result = RecvRaw();
		if(!result)
			return result.error();
		
		recvbuffer.Recv(result->data(), result->size());
	}

	if(timeout > 0.0f)
		SetTimeout();
		
	std::vector<byte> result = recvbuffer.GetMsg();
	if(isSecure)
		result = decryptor.Crypt(result);
	return ByteQueue(result);
}

ErrorCode MyClientSocket::Send(ByteQueue bytes)
{
	return Send(bytes.vector());
}

ErrorCode MyClientSocket::Send(std::vector<byte> bytes)
{
	if(isSecure)
		return SendRaw(PacketProcessor::encapsulate(encryptor.Crypt(bytes)));
	return SendRaw(PacketProcessor::encapsulate(bytes));
}

ErrorCode MyClientSocket::SendRaw(const std::vector<byte> &bytes)
{
	return SendRaw(bytes.data(), bytes.size());
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