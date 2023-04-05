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
	return Send(bytes.vector());
}

ErrorCode MyClientSocket::Send(std::vector<byte> bytes)
{
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
		
	if(ec.typecode() == ErrorCode::TYPE_ERRNO && ec.code() == EINVAL)	return true;
	if(ec.typecode() == ErrorCode::TYPE_CUSTOM && ec.code() == ERR_CONNECTION_CLOSED) return true;
	return false;
}