#pragma once
#include "MyServer.hpp"
#include "MyConnectee.hpp"
#include "MyConnector.hpp"
#include "ConfigParser.hpp"
#include "MyCodes.hpp"
#include "MyMatchMaker.hpp"
#include "MyNotifyMsgs.hpp"
#include "MyPostgres.hpp"
#include "DeMap.hpp"

using mylib::utils::Notifier;
using mylib::utils::MyNotifyTarget;
using mylib::utils::DeMap;
using mylib::threads::Thread;

class MyMatch : public MyServer
{
	private:
		static constexpr Seed_t MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		const std::chrono::seconds rematch_delay = std::chrono::seconds(1);	//TODO : Config로 뺄 필요가 있음.
		DeMap<Account_ID_t, Hash_t, std::shared_ptr<Notifier>> sessions;

		MyConnectee connectee;
		MyConnector connector_battle;

		MyMatchMaker matchmaker;
		Thread t_matchmaker;

	public:
		MyMatch();	//TODO : Join에서 t_matchmaker가 exception으로 죽으면 다시 올려야 한다.
		~MyMatch();
		void Open() override;
		void Close() override;
		void ClientProcess(std::shared_ptr<MyClientSocket>) override;
	
	private:
		ByteQueue MatchInquiry(ByteQueue);
		void MatchMake(std::shared_ptr<bool>);
};