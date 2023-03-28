#pragma once
#include <string>
#include <pqxx/pqxx>
#include <memory>
#include <thread>
#include <chrono>
#include <tuple>
#include <map>
#include "ConfigParser.hpp"
#include "StackTraceExcept.hpp"
#include "Logger.hpp"

using mylib::utils::ConfigParser;
using mylib::utils::StackTraceExcept;
using mylib::utils::Logger;

class MyPostgres
{
	private:
		static bool isRunning;

		static std::unique_ptr<pqxx::connection> db_c;
		static std::mutex db_mtx;

		std::unique_ptr<pqxx::work> db_w;
		std::unique_lock<std::mutex> db_lk;

	public:
		static void Open();
		static void Close();
		static bool isOpened();
	public:
		MyPostgres();
		MyPostgres(const MyPostgres&) = delete;
		MyPostgres(MyPostgres&&) = delete;
		MyPostgres &operator=(const MyPostgres&) = delete;
		MyPostgres& operator=(MyPostgres&&) = delete;
		~MyPostgres();
		void abort();
		void commit();
		std::string quote(std::string);
		std::string quote_name(std::string);
		std::string quote_raw(const unsigned char*, size_t);
		pqxx::result exec(const std::string& query);
		pqxx::row exec1(const std::string& query);
		std::tuple<Achievement_ID_t, int, int, int> GetInfo(Account_ID_t);
		std::map<Achievement_ID_t, int> GetAllAchieve(Account_ID_t);
		bool SetNickName(Account_ID_t, Achievement_ID_t);
		void IncreaseScore(Account_ID_t, int);
		bool Achieve(Account_ID_t, Achievement_ID_t);
		bool AchieveProgress(Account_ID_t, Achievement_ID_t, int);

	private:
		std::tuple<int, int> GetAchieve(Account_ID_t, Achievement_ID_t);
};