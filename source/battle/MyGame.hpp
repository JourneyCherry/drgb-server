#pragma once
#include <mutex>
#include <condition_variable>
#include <map>
#include <chrono>
#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include "Expected.hpp"
#include "MyCodes.hpp"

using mylib::utils::Expected;

#ifdef __linux__
	#define MYPOPCNT(x)	__builtin_popcount(x)
#elif _WIN32 || _WIN64
	#define MYPOPCNT(x)	__popcnt(x)
#endif

class MyGame
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		std::mutex mtx;
		
		int now_round;

		static const std::map<int, int> required_energy;

		std::queue<std::tuple<int, Achievement_ID_t, int>> Achievements;

	public:
		static constexpr int MAX_PLAYER = 2;
		static constexpr int MAX_HEALTH = 1;
		static constexpr int MAX_ENERGY = 2;
		static constexpr int MAX_ROUND = 10;
		static constexpr int DODGE_ROUND = 2;

		struct player_info
		{
			int action;
			int health;
			int energy;
			int target;
			//For Record Achievement
			int prev_action;
			int consecutive;
		} players[MAX_PLAYER];

		static constexpr int MEDITATE = 1 << 0;
		static constexpr int GUARD = 1 << 1;
		static constexpr int EVADE = 1 << 2;
		static constexpr int PUNCH = 1 << 3;
		static constexpr int FIRE = 1 << 4;

		MyGame();
		~MyGame() = default;
		void Action(int, int);
		int GetWinner() const;
		bool process();
		int GetRound() const;
		std::tuple<byte, int, int> GetPlayerInfo(const int&) const;
		Expected<std::tuple<int, Achievement_ID_t, int>> GetAchievement();
	
	private:
		void ProcessResult(bool, bool);
		bool CheckAction(int);
		void AchieveProgress(int, Achievement_ID_t, int);
};