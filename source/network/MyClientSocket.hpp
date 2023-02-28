#pragma once
#include <string>
#include "Expected.hpp"
#include "PacketProcessor.hpp"

using mylib::utils::Expected;
using mylib::utils::PacketProcessor;
using mylib::utils::ByteQueue;

class MyClientSocket
{
	protected:
		std::string Address;
		PacketProcessor recvbuffer;

	public:
		MyClientSocket(){}
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		virtual Expected<ByteQueue, int> Recv() = 0;
		virtual Expected<int> Send(ByteQueue) = 0;	//Value가 ErrorCode가 된다.
		virtual void Close() = 0;

		std::string ToString();
};