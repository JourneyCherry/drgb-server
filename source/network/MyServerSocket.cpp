#include "MyServerSocket.hpp"

ErrorCode MyServerSocket::GetSSLError()
{
	return ErrorCode(ERR_get_error());
}