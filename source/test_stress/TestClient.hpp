#pragma once
#include <string>
#include <future>
#include <vector>
#include <utility>
#include <chrono>
#include <random>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include "MyWebsocketClient.hpp"
#include "ThreadExceptHandler.hpp"
#include "Hasher.hpp"

using mylib::security::Hasher;
using mylib::threads::ThreadExceptHandler;

class TestClient
{
	public:
		static std::vector<std::pair<std::string, int>> addrs;
		static ThreadExceptHandler *parent;
		static boost::asio::io_context *pioc;
		static const int LoginTimeout;

		static std::random_device rd;
		static std::mt19937_64 gen;
		static std::uniform_int_distribution<int> dis;
		static std::uniform_int_distribution<int> millidis;

		static bool doNotRestart;
		static bool isRunning;
		static constexpr int STATE_CLOSED = -1;
		static constexpr int STATE_RESTART = 0;
		static constexpr int STATE_LOGIN = 1;
		static constexpr int STATE_MATCH = 2;
		static constexpr int STATE_BATTLE = 3;
		static constexpr int STATE_CLOSING = 4;
		
		static void SetRestart(const bool&);
		static void AddAddr(std::string, int);
		static void SetInfo(ThreadExceptHandler*, boost::asio::io_context*);
		static int GetRandomAction();
		static std::string GetStateName(int);

	private:
		std::shared_ptr<MyWebsocketClient> socket;
		std::shared_ptr<boost::asio::steady_timer> timer;

		//Client State
		int number;
		int now_state;

		//For Login
		std::string id;
		std::string pwd;

		//For Access
		Hash_t cookie;
		Seed_t battle_seed;

		//Player Info
		Achievement_ID_t nickname;
		int win;
		int draw;
		int loose;

		byte action;
		int health;
		int energy;

		//Enemy Info
		Achievement_ID_t enemy_nickname;
		int enemy_win;
		int enemy_draw;
		int enemy_loose;
		
		byte enemy_action;
		int enemy_health;
		int enemy_energy;

		//Game Info
		int round;

		std::mutex last_error_mtx;
		std::string last_error;
		void RecordError(std::string, std::string, int, std::string);

		std::mutex delay_mtx;
		double delay_auth;
		double delay_match;
		double delay_battle;
		double delay_reserved;
		std::chrono::system_clock::time_point delay_start;
		void StartRecord(double=0.0);
		void GetDelay(int, double=0.0);

	public:
		TestClient(int);
		~TestClient() = default;
		void DoLogin();
		void DoMatch();
		void DoBattle();
		void DoRestart();
		void Close();
		void Shutdown();
		int GetId();
		Seed_t GetSeed();
		int GetState();
		bool isConnected();
		std::string GetLastError() const;
		void ClearLastError();
		std::tuple<double, double, double> GetDelays() const;
	
	protected:
		void LoginProcess();
		void AccessProcess(int, std::function<void(std::shared_ptr<MyClientSocket>)>);
		void AuthProcess(std::shared_ptr<MyClientSocket>);
		void MatchProcess(std::shared_ptr<MyClientSocket>);
		void BattleProcess(std::shared_ptr<MyClientSocket>);
};