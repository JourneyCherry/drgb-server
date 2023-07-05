#pragma once
#include <array>
#include <functional>
#include <memory>
#include "MyServer.hpp"
#include "MyGame.hpp"
#include "DeMap.hpp"
#include "MyConnectee.hpp"
#include "MyConnectorPool.hpp"
#include "TimerPool.hpp"
#include "MyConnectee.hpp"
#include "MyRedis.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::DeMap;
using mylib::threads::FixedPool;
using mylib::threads::TimerPool;

class MyBattle : public MyServer
{
	private:
		static constexpr int MAX_GAME = MAX_CLIENTS / 2;	//std::floor는 C++23에서 constexpr이며, 이전엔 일반 함수이다.

		MyConnectee connectee;
		MyConnectorPool connector;

		Seed_t machine_id;
		std::string keyword_match;
		std::string self_keyword;

		MyRedis redis;

		DeMap<Account_ID_t, Hash_t, std::shared_ptr<MyGame>> sessions;
		//FixedPool<std::shared_ptr<MyGame>, MAX_GAME> gamepool;
		TimerPool gamepool;
	
	public:
		MyBattle();
		~MyBattle();
		void Open() override;
		void Close() override;

	private:
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void EnterProcess(std::shared_ptr<MyClientSocket>, ErrorCode);
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode, Account_ID_t, int, std::shared_ptr<MyGame>);
		void GameProcess(std::shared_ptr<boost::asio::steady_timer>, std::shared_ptr<MyGame>, const boost::system::error_code&);
		void SessionProcess(std::shared_ptr<MyClientSocket>, const Account_ID_t&, const Hash_t&);

		ByteQueue BattleInquiry(ByteQueue);
		ByteQueue MgrInquiry(ByteQueue);
};