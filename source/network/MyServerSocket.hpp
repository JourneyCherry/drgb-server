#pragma once
#include <memory>
#include "MyClientSocket.hpp"

using mylib::utils::Expected;

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

		virtual Expected<std::shared_ptr<MyClientSocket>> Accept() = 0;
		virtual void Close() = 0;
};