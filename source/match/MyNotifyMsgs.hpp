#pragma once
#include "MyBytes.hpp"
#include "MyNotifier.hpp"

class MyDisconnectMessage : public MyNotifyTarget
{
	public:
		static constexpr int Message_ID = 0;
		MyDisconnectMessage(){}
		~MyDisconnectMessage() = default;
		int Type() override { return Message_ID;}
};

class MyClientMessage : public MyNotifyTarget
{
	public:
		static constexpr int Message_ID = 1;
		MyBytes message;
		MyClientMessage(MyBytes data) : message(data){}
		~MyClientMessage() = default;
		int Type() override { return Message_ID;}
};

class MyMatchMakerMessage : public MyNotifyTarget
{
	public:
		static constexpr int Message_ID = 2;
		int BattleServer;
		MyMatchMakerMessage(int server_id) : BattleServer(server_id) {}
		~MyMatchMakerMessage() = default;
		int Type() override { return Message_ID;}
};