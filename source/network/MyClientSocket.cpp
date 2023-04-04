#include "MyClientSocket.hpp"

std::string MyClientSocket::ToString()
{
	return Address;
}

Expected<ByteQueue, ErrorCode> MyClientSocket::Recv()
{
	auto result = RecvRaw();
	if(!result)
		return result.error();
	return ByteQueue(PacketProcessor::decapsulate(*result));
}

ErrorCode MyClientSocket::Send(ByteQueue bytes)
{
	return SendRaw(PacketProcessor::encapsulate(bytes.vector()));
}

ErrorCode MyClientSocket::SendRaw(const std::vector<byte> &bytes)
{
	return SendRaw(bytes.data(), bytes.size());
}

bool MyClientSocket::isNormalClose(const ErrorCode &ec)
{
	if(ec)
		return true;
	return (ec.typecode() == ErrorCode::TYPE_CUSTOM && ec.code() == ERR_CONNECTION_CLOSED);
}