#include "MyClientSocket.hpp"

std::string MyClientSocket::ToString()
{
	return Address;
}

bool MyClientSocket::isNormalClose(const ErrorCode &ec)
{
	return (ec.typecode() == ErrorCode::TYPE_CUSTOM && ec.code() == ERR_CONNECTION_CLOSED);
}