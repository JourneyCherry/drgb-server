#pragma once
#include "MyServer.hpp"
#include "MyConnectee.hpp"
#include "MyConnector.hpp"
#include "ConfigParser.hpp"
#include "MyCodes.hpp"
#include "MyMatchMaker.hpp"
#include "MyNotifyMsgs.hpp"
#include "MyPostgres.hpp"
#include "MyRedis.hpp"
#include "DeMap.hpp"

using mylib::utils::Notifier;
using mylib::utils::NotifyTarget;
using mylib::utils::DeMap;
using mylib::threads::Thread;
using mylib::utils::Expected;

class MyMatch : public MyServer
{
	private:
		static const std::string self_keyword;
		const std::chrono::seconds rematch_delay = std::chrono::seconds(1);	//TODO : Config로 뺄 필요가 있음.
		DeMap<Account_ID_t, Hash_t, std::weak_ptr<MyClientSocket>> sessions;	//Local Server Session

		MyConnectee connectee;

		MyMatchMaker matchmaker;
		Thread t_matchmaker;

		MyRedis redis;

	public:
		MyMatch();	//TODO : Join에서 t_matchmaker가 exception으로 죽으면 다시 올려야 한다.
		~MyMatch();
		void Open() override;
		void Close() override;

	private:
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void EnterProcess(std::shared_ptr<MyClientSocket>, ErrorCode);
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode, Account_ID_t);
		void SessionProcess(std::shared_ptr<MyClientSocket>, const Account_ID_t&, const Hash_t&);
	
		ByteQueue MatchInquiry(ByteQueue);
		ByteQueue MgrInquiry(ByteQueue);
		void MatchMake();
};