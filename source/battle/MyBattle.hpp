#pragma once
#include <array>
#include <functional>
#include <memory>
#include "MyServer.hpp"
#include "MyGame.hpp"
#include "MyCodes.hpp"
#include "DeMap.hpp"
#include "MyConnector.hpp"
#include "MyConnectee.hpp"
#include "Pool.hpp"
#include "Thread.hpp"

using mylib::utils::DeMap;

class MyBattle : public MyServer
{
	private:
		static constexpr Seed_t MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		static constexpr int MAX_GAME = MAX_CLIENTS / 2;	//std::floor는 C++23에서 constexpr이며, 이전엔 일반 함수이다.

		MyConnectee connectee;
		MyConnector connector_match;

		DeMap<Account_ID_t, Hash_t, std::shared_ptr<MyGame>> cookies;
		mylib::threads::Thread poolManager;
		mylib::threads::Pool gamepool;
	
	public:
		MyBattle();
		~MyBattle();
		void Open() override;
		void Close() override;
		void ClientProcess(std::shared_ptr<MyClientSocket>) override;

	private:
		ByteQueue BattleInquiry(ByteQueue);
		void pool_manage(std::shared_ptr<bool>);
};