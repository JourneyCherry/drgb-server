#pragma once
#include <memory>
#include "MyExpected.hpp"
#include "MyClientSocket.hpp"

class MyServerSocket
{
	protected:
		int port;

	public:
		MyServerSocket(int p) : port(p){}
		MyServerSocket(const MyServerSocket&) = delete;
		MyServerSocket(MyServerSocket&&) = delete;
		virtual ~MyServerSocket() = default;

		MyServerSocket& operator=(const MyServerSocket&) = delete;
		MyServerSocket& operator=(MyServerSocket&&) = delete;

		virtual MyExpected<std::shared_ptr<MyClientSocket>, int> Accept() = 0;
		virtual void Close() = 0;
};