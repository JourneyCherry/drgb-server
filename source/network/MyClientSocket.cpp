#include "MyClientSocket.hpp"

std::string MyClientSocket::ToString()
{
	return Address;
}

Expected<ByteQueue, ErrorCode> MyClientSocket::Recv()
{
	while(!recvbuffer.isMsgIn())
	{
		auto result = RecvRaw();
		if(!result)
			return result.error();
		
		recvbuffer.Recv(result->data(), result->size());
	}

	return ByteQueue(PacketProcessor::decapsulate(recvbuffer.GetMsg()));
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

ErrorCode MyClientSocket::GetSSLError()
{
	return ErrorCode(ERR_get_error());
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
	}
	return false;
}