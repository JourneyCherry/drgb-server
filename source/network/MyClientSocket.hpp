#pragma once
#include <string>
#include <openssl/err.h>
#include "Expected.hpp"
#include "ByteQueue.hpp"
#include "PacketProcessor.hpp"
#include "ConfigParser.hpp"
#include "KeyExchanger.hpp"
#include "Encryptor.hpp"

using mylib::utils::Expected;
using mylib::utils::PacketProcessor;
using mylib::utils::ByteQueue;
using mylib::utils::ErrorCode;
using mylib::utils::StackErrorCode;
using mylib::utils::ConfigParser;
using mylib::security::Encryptor;
using mylib::security::KeyExchanger;

class MyClientSocket
{
	private:
		bool isSecure;
		Encryptor encryptor, decryptor;
		static constexpr float TIME_KEYEXCHANGE = 2.0f;

	protected:
		std::string Address;
		PacketProcessor recvbuffer;

	public:
		MyClientSocket();
		MyClientSocket(const MyClientSocket&) = delete;
		MyClientSocket(MyClientSocket&&) = delete;
		virtual ~MyClientSocket() = default;

		virtual StackErrorCode Connect(std::string, int) = 0;
		virtual void Close() = 0;
		virtual void SetTimeout(float = 0.0f) = 0;	//seconds

		MyClientSocket& operator=(const MyClientSocket&) = delete;
		MyClientSocket& operator=(MyClientSocket&&) = delete;

		Expected<ByteQueue, ErrorCode> Recv(float = 0.0f);
		ErrorCode Send(ByteQueue);
		ErrorCode Send(std::vector<byte>);

		std::string ToString();
		StackErrorCode KeyExchange(bool = true);

		static bool isNormalClose(const ErrorCode&);

	protected:
		virtual Expected<std::vector<byte>, ErrorCode> RecvRaw() = 0;
		virtual ErrorCode SendRaw(const byte*, const size_t&) = 0;
		ErrorCode SendRaw(const std::vector<byte>&);
};