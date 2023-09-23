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

/**
 * @brief Game Information. It takes parts of a 'Game Session'.
 * 
 */
class GameInfo
{
	public:
		std::mutex mtx;
		std::shared_ptr<MyGame> game;
		std::shared_ptr<boost::asio::steady_timer> timer;
		DeMap<Account_ID_t, int, std::weak_ptr<MyClientSocket>> session;
		std::array<ByteQueue, MyGame::MAX_PLAYER> PlayerInfos;
		bool Previous_AllIn;
		/**
		 * @brief Constructor of Game Information.
		 * 
		 * @param game_ actual Game Logic Object.
		 * @param timer_ Timer Object to proceed game or wait players.
		 */
		GameInfo(std::shared_ptr<MyGame> game_, std::shared_ptr<boost::asio::steady_timer> timer_) : game(game_), timer(timer_), Previous_AllIn(false) {}
		/**
		 * @brief Is All player Connected now?
		 * 
		 * @return true 
		 * @return false 
		 */
		bool isAllIn();
		/**
		 * @brief 'id' player just has been connected. If there are no matching id, return failure.
		 * 
		 * @param id connected player's id
		 * @param socket connected player's socket
		 * @return std::weak_ptr<MyClientSocket> - reference pointer of player's socket that stores in this Game Session.
		 * @return fail - there are no ID registered on this Game Session.
		 * 
		 */
		Expected<std::weak_ptr<MyClientSocket>> Enter(Account_ID_t id, std::shared_ptr<MyClientSocket> socket);
		/**
		 * @brief 'id' player just has been disconnected.
		 * 
		 * @param id disconnected player's id
		 * @param socket disconnected player's socket
		 */
		void Exit(Account_ID_t id, std::shared_ptr<MyClientSocket> socket);
};

/**
 * @brief Battle Server.
 * **Spec**
 *  - Socket Type : Websocket
 *  - Process
 *   - Receive Cookie and Check it with 'Redis'. If found, Send Player & Game Information, or Reject Connection
 *   - If two player is connected, Process Game until finish.
 *   - If one of players is disconnected in the middle of the game, Game Pause for some time or the player is reconnected. If not, crashing game or left player will win.
 *   - When Game Finished, Record it on DB and transfer players to 'Match Server' with game results.
 */
class MyBattle : public MyServer
{
	private:
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
		/**
		 * @brief Constructor of Battle Server.
		 * 
		 */
		MyBattle();
		~MyBattle();
		void Open() override;
		void Close() override;

	private:
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, Account_ID_t, int, std::shared_ptr<MyGame>);
		/**
		 * @brief Session Process for HeartBeat and Redis.
		 * 
		 * @param target_client client socket
		 * @param account_id account's id
		 * @param cookie client's cookie value
		 * 
		 */
		void SessionProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, Hash_t cookie);

		/**
		 * @brief Actual Game Play Logic. it controls player's disconnection too.
		 * 
		 * @param timer timer object to wait.
		 * @param gameinfo State of the game. it includes player's connection status.
		 */
		void GameProcess(std::shared_ptr<boost::asio::steady_timer> timer, std::shared_ptr<GameInfo> gameinfo);

		/**
		 * @brief gRPC function of 'MatchTransfer()' from Match Server. Create new game with 2 players and send back usage.
		 * 
		 * @param lpid left player's account id
		 * @param rpid right player's account id
		 * 
		 */
		void MatchTransfer(const Account_ID_t &lpid, const Account_ID_t &rpid);

		/**
		 * @brief Send information of the game to participated players. It includes round result, disconnected, etc not only game result.
		 * 
		 * @param msg information of the game.
		 * @param gameinfo game's information.
		 */
		void SendRoundResult(const byte &msg, std::shared_ptr<GameInfo> gameinfo);
		/**
		 * @brief record achievement's progress on DB and send it to player's client.
		 * 
		 * @param id player's account id
		 * @param achieve achievement id that the player has achieved
		 * @param count achievement counter
		 * @param socket player's socket to send the achievement information
		 */
		void AchieveProgress(Account_ID_t id, Achievement_ID_t achieve, int count, std::shared_ptr<MyClientSocket> socket);
	
	protected:
		size_t GetUsage();
		bool CheckAccount(Account_ID_t);
		std::map<std::string, size_t> GetConnectUsage();
};