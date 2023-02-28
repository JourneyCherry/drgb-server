#pragma once
#include "ByteQueue.hpp"
#include "Notifier.hpp"

using mylib::utils::NotifyTarget;

class MyDisconnectMessage : public NotifyTarget
{
	public:
		static constexpr int Message_ID = 0;
		MyDisconnectMessage(){}
		~MyDisconnectMessage() = default;
		int Type() override { return Message_ID;}
};

class MyClientMessage : public NotifyTarget
{
	public:
		static constexpr int Message_ID = 1;
		ByteQueue message;
		MyClientMessage(ByteQueue data) : message(data){}
		~MyClientMessage() = default;
		int Type() override { return Message_ID;}
};

class MyMatchMakerMessage : public NotifyTarget
{
	public:
		static constexpr int Message_ID = 2;
		int BattleServer;
		MyMatchMakerMessage(int server_id) : BattleServer(server_id) {}
		~MyMatchMakerMessage() = default;
		int Type() override { return Message_ID;}
};