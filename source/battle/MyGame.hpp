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

/**
 * @brief Actual Game Process and Algorithm. one 'MyGame' object controls one 'Battle'.
 * 
 */
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

		/**
		 * @brief Constructor of game. Starts with round 0 with 2 players(player 0 and player 1).
		 * 
		 */
		MyGame();
		~MyGame() = default;

		/**
		 * @brief Reserve action to player.
		 * 
		 * @param side which player to act 'action'. it should 0(left) or 1(right).
		 * @param action action the player will act. If it isn't registered action type, ignore this request.
		 */
		void Action(int side, int action);
		/**
		 * @brief Get winner player. If the game is still playing or draw, it returns -1.
		 * 
		 * @return int win player's number(0 or 1). -1 for draw or not done.
		 */
		int GetWinner() const;
		/**
		 * @brief Process one round and get whether the game is finished or not.
		 * 
		 * @return true if the game is over.
		 * @return false if the game is still running.
		 */
		bool process();
		/**
		 * @brief Get current round number.
		 * 
		 * @return int round number
		 */
		int GetRound() const;
		/**
		 * @brief Get the player's current game information. If player number is wrong, returns {0, -1, -1}.
		 * 
		 * @param side player number.
		 * @return byte - action type of previous round.
		 * @return int - player's current health
		 * @return int - player's current energy 
		 */
		std::tuple<byte, int, int> GetPlayerInfo(const int &side) const;
		/**
		 * @brief Get achievements from achievement queue. If there are no achievement, return fail. should call this until it returns fail.
		 * 
		 * @return int - player number.
		 * @return Achievement_ID_t - achievement number.
		 * @return int - achievement counter.
		 * @return failure - there are no achievement.
		 */
		Expected<std::tuple<int, Achievement_ID_t, int>> GetAchievement();
	
	private:
		void ProcessResult(bool, bool);
		bool CheckAction(int);
		void AchieveProgress(int, Achievement_ID_t, int);
};