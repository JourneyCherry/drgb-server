#pragma once
#include "MyServer.hpp"
#include "MatchServiceServer.hpp"
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
		std::chrono::milliseconds rematch_delay;
		DeMap<Account_ID_t, Hash_t, std::weak_ptr<MyClientSocket>> sessions;	//Local Server Session

		MatchServiceServer MatchService;
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
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, Account_ID_t);
		void SessionProcess(std::shared_ptr<MyClientSocket>, Account_ID_t, Hash_t);

		void MatchMake();

	protected:
		bool CheckAccount(Account_ID_t) override;
		std::map<std::string, size_t> GetConnectUsage() override;
};