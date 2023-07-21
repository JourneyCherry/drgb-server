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

class GameInfo
{
	public:
		std::mutex mtx;
		std::shared_ptr<MyGame> game;
		std::shared_ptr<boost::asio::steady_timer> timer;
		DeMap<Account_ID_t, int, std::weak_ptr<MyClientSocket>> session;
		std::array<ByteQueue, 2> PlayerInfos;
		bool Previous_AllIn;
		GameInfo(std::shared_ptr<MyGame> game_, std::shared_ptr<boost::asio::steady_timer> timer_) : game(game_), timer(timer_), Previous_AllIn(false) {}
		bool isAllIn();
		Expected<std::weak_ptr<MyClientSocket>> Enter(Account_ID_t, std::shared_ptr<MyClientSocket>);
		void Exit(Account_ID_t, std::shared_ptr<MyClientSocket>);
};

class MyBattle : public MyServer
{
	private:
		static constexpr int MAX_GAME = MAX_CLIENTS / 2;	//std::floor는 C++23에서 constexpr이며, 이전엔 일반 함수이다.

		std::chrono::milliseconds Round_Time;
		std::chrono::milliseconds Dis_Time;	//디스 시간.
		std::chrono::milliseconds StartAnim_Time;	//경기 시작/재개 시, 애니메이션 시간. Start Animation이 그려진 뒤, 자동으로 한 라운드를 진행하므로 사용자 입력을 기다리는 Round_Time이 추가되어야 한다.

		Seed_t machine_id;
		std::string self_keyword;
		std::string keyword_match;

		BattleServiceClient BattleService;	//Match To Battle Service Client

		DeMap<Account_ID_t, Hash_t, std::shared_ptr<GameInfo>> sessions;
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
		void SessionProcess(std::shared_ptr<MyClientSocket>, Account_ID_t, Hash_t);

		void GameProcess(std::shared_ptr<boost::asio::steady_timer>, std::shared_ptr<GameInfo>);

		void MatchTransfer(const Account_ID_t&, const Account_ID_t&);

		void SendRoundResult(const byte&, std::shared_ptr<GameInfo>);
		void AchieveProgress(Account_ID_t, Achievement_ID_t, int, std::shared_ptr<MyClientSocket>);
	
	protected:
		size_t GetUsage();
		bool CheckAccount(Account_ID_t);
		std::map<std::string, size_t> GetConnectUsage();
};