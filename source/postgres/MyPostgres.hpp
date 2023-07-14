#pragma once
#include <string>
#include <pqxx/pqxx>
#include <memory>
#include <thread>
#include <chrono>
#include <tuple>
#include <map>
#include "StackTraceExcept.hpp"
#include "Logger.hpp"
#include "Hasher.hpp"
#include "Encoder.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::Logger;
using mylib::security::Hasher;
using mylib::utils::ByteQueue;
using mylib::utils::Encoder;

class MyPostgres
{
	private:
		std::unique_ptr<pqxx::work> work;
		std::function<void()> releaseConnection;

		std::string quote(std::string);
		std::string quote_name(std::string);
		std::string quote_raw(const unsigned char*, size_t);
		pqxx::result exec(const std::string& query);
		pqxx::row exec1(const std::string& query);
		
	public:
		MyPostgres(pqxx::connection*, std::function<void()>);
		MyPostgres(const MyPostgres&) = delete;
		MyPostgres(MyPostgres&&) = delete;
		MyPostgres &operator=(const MyPostgres&) = delete;
		MyPostgres& operator=(MyPostgres&&) = delete;
		~MyPostgres();
		void abort();
		void commit();
		Account_ID_t RegisterAccount(std::string, Pwd_Hash_t);
		Account_ID_t FindAccount(std::string, Pwd_Hash_t);
		bool ChangePwd(Account_ID_t, Pwd_Hash_t, Pwd_Hash_t);
		std::tuple<Achievement_ID_t, int, int, int> GetInfo(Account_ID_t);
		std::map<Achievement_ID_t, int> GetAllAchieve(Account_ID_t);
		bool SetNickName(Account_ID_t, Achievement_ID_t);
		bool Achieve(Account_ID_t, Achievement_ID_t);
		bool AchieveProgress(Account_ID_t, Achievement_ID_t, int);
		void ArchiveBattle(Account_ID_t, Account_ID_t, bool, bool);

	private:
		std::tuple<int, int> GetAchieve(Account_ID_t, Achievement_ID_t);
		Hash_t GetPwdHash(Account_ID_t, Pwd_Hash_t, std::string);
};