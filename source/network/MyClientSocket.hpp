#pragma once
#include <string>
#include "Expected.hpp"
#include "PacketProcessor.hpp"

using mylib::utils::Expected;
using mylib::utils::PacketProcessor;
using mylib::utils::ByteQueue;
using mylib::utils::ErrorCode;

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

		virtual Expected<ByteQueue, ErrorCode> Recv() = 0;
		virtual ErrorCode Send(ByteQueue) = 0;	//Value가 ErrorCode가 된다.
		virtual void Close() = 0;

		std::string ToString();

		static bool isNormalClose(const ErrorCode&);
};