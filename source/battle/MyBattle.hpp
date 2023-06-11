#pragma once
#include <array>
#include <functional>
#include <memory>
#include "MyServer.hpp"
#include "MyGame.hpp"
#include "DeMap.hpp"
#include "MyConnectorPool.hpp"
#include "TimerPool.hpp"
#include "MyConnectee.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::DeMap;
using mylib::threads::FixedPool;
using mylib::threads::TimerPool;

class MyBattle : public MyServer
{
	private:
		static constexpr Seed_t MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		static constexpr int MAX_GAME = MAX_CLIENTS / 2;	//std::floor는 C++23에서 constexpr이며, 이전엔 일반 함수이다.

		MyConnectorPool connector;
		std::string keyword_match;

		DeMap<Account_ID_t, Hash_t, std::shared_ptr<MyGame>> cookies;
		//FixedPool<std::shared_ptr<MyGame>, MAX_GAME> gamepool;
		TimerPool gamepool;
	
	public:
		MyBattle();
		~MyBattle();
		void Open() override;
		void Close() override;
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void EnterProcess(std::shared_ptr<MyClientSocket>, ErrorCode);
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode, Account_ID_t, int, std::shared_ptr<MyGame>);
		void GameProcess(std::shared_ptr<boost::asio::steady_timer>, std::shared_ptr<MyGame>, const boost::system::error_code&);

	private:
		ByteQueue BattleInquiry(ByteQueue);
};