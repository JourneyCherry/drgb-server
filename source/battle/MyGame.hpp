#pragma once
#include <mutex>
#include <condition_variable>
#include <map>
#include <chrono>
#include "MyCodes.hpp"
#include "MyPostgres.hpp"
#include "MyClientSocket.hpp"
#include "PoolElement.hpp"

#ifdef __linux__
	#define MYPOPCNT(x)	__builtin_popcount(x)
#elif _WIN32 || _WIN64
	#define MYPOPCNT(x)	__popcnt(x)
#endif

class MyGame : public mylib::threads::PoolElement
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		std::mutex mtx;
		std::condition_variable cv;

		static const std::chrono::seconds Round_Time;
		static const std::chrono::seconds Dis_Time;	//디스 시간.
		static const std::map<int, int> required_energy;


	public:
		static constexpr int MAX_PLAYER = 2;
		static constexpr int MAX_HEALTH = 1;
		static constexpr int MAX_ENERGY = 2;
		static constexpr int MAX_ROUND = 10;
		static constexpr int DODGE_ROUND = 3;

		struct player_info
		{
			Account_ID_t id;
			std::shared_ptr<MyClientSocket> socket;
			int action;
			int health;
			int energy;
			int target;
			byte result;
		} players[MAX_PLAYER];

		static constexpr int MEDITATE = 1 << 0;
		static constexpr int GUARD = 1 << 1;
		static constexpr int EVADE = 1 << 2;
		static constexpr int PUNCH = 1 << 3;
		static constexpr int FIRE = 1 << 4;

		MyGame(Account_ID_t, Account_ID_t);
		~MyGame() = default;
		int Connect(Account_ID_t, std::shared_ptr<MyClientSocket>);
		void Disconnect(int, std::shared_ptr<MyClientSocket>);
		void Action(int, int);
		int GetWinner();
	
	private:
		void Work() override;
		bool process();
		bool CheckAction(int);
		void SendAll(MyBytes);	//같은 패킷 보낼때 씀.
		MyBytes GetPlayerByte(const struct player_info&);
};