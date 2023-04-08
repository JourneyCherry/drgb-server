#pragma once
#include <string>
#include <openssl/err.h>
#include "Expected.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "ConfigParser.hpp"

using mylib::utils::Expected;
using mylib::utils::PacketProcessor;
using mylib::utils::ByteQueue;
using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ConfigParser;

class MyClientSocket
{
	protected:
		std::string Address;
		PacketProcessor recvbuffer;

		static ErrorCode GetSSLError();

	public:
		MyClientSocket() {}
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		Expected<ByteQueue, ErrorCode> Recv();
		ErrorCode Send(ByteQueue);
		ErrorCode Send(std::vector<byte>);
		virtual void Close() = 0;

		std::string ToString();

		virtual StackErrorCode Connect(std::string, int) = 0;
		static bool isNormalClose(const ErrorCode&);

	protected:
		virtual Expected<std::vector<byte>, ErrorCode> RecvRaw() = 0;
		virtual ErrorCode SendRaw(const byte*, const size_t&) = 0;
		ErrorCode SendRaw(const std::vector<byte>&);
};