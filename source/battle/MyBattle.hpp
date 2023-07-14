#pragma once
#include <array>
#include <functional>
#include <memory>
#include "MyServer.hpp"
#include "MyGame.hpp"
#include "DeMap.hpp"
#include "TimerPool.hpp"
#include "BattleServiceClient.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::DeMap;
using mylib::threads::TimerPool;

class MyBattle : public MyServer
{
	private:
		static constexpr int MAX_GAME = MAX_CLIENTS / 2;	//std::floor는 C++23에서 constexpr이며, 이전엔 일반 함수이다.

		Seed_t machine_id;
		std::string self_keyword;
		std::string keyword_match;

		BattleServiceClient BattleService;	//Match To Battle Service Client

		DeMap<Account_ID_t, Hash_t, std::shared_ptr<MyGame>> sessions;
		TimerPool gamepool;
	
	public:
		MyBattle();
		~MyBattle();
		void Open() override;
		void Close() override;

	private:
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, Account_ID_t, int, std::shared_ptr<MyGame>);
		void GameProcess(std::shared_ptr<boost::asio::steady_timer>, std::shared_ptr<MyGame>, const boost::system::error_code&);
		void SessionProcess(std::shared_ptr<MyClientSocket>, Account_ID_t, Hash_t);

		void MatchTransfer(const Account_ID_t&, const Account_ID_t&);
	
	protected:
		size_t GetUsage();
		bool CheckAccount(Account_ID_t);
		std::map<std::string, size_t> GetConnectUsage();
};