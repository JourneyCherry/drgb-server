#pragma once
#include "MyServer.hpp"
#include "MyConnectee.hpp"
#include "MyConnector.hpp"
#include "MyConfigParser.hpp"
#include "MyCodes.hpp"
#include "MyMatchMaker.hpp"
#include "MyNotifyMsgs.hpp"
#include "MyPostgres.hpp"
#include "MyDeMap.hpp"

class MyMatch : public MyServer
{
	private:
		static constexpr unsigned int MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		MyDeMap<Account_ID_t, Hash_t, std::shared_ptr<MyNotifier>> sessions;

		MyConnectee connectee;
		MyConnector connector_battle;

		MyMatchMaker matchmaker;
		mylib::threads::Thread t_matchmaker;

	public:
		MyMatch();
		~MyMatch();
		void Open() override;
		void Close() override;
		void ClientProcess(std::shared_ptr<MyClientSocket>) override;
	
	private:
		MyBytes MatchInquiry(MyBytes);
		void MatchMake(std::shared_ptr<bool>);
};