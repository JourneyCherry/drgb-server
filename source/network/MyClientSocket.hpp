#pragma once
#include <string>
#include "MyExpected.hpp"
#include "MyMsg.hpp"

class MyClientSocket
{
	protected:
		std::string Address;
		MyMsg recvbuffer;

	public:
		MyClientSocket(){}
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		virtual MyExpected<MyBytes, int> Recv() = 0;
		virtual MyExpected<int> Send(MyBytes) = 0;	//Value가 ErrorCode가 된다.
		virtual void Close() = 0;

		std::string ToString();
};