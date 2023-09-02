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

/**
 * @brief PostgreSQL Worker Object. It should be controlled by only one thread. Never Create This Class. It should be created in @ref MyPostgresPool Only.
 * 
 */
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
		/**
		 * @brief NEVER CREATE THIS CLASS. It should be created in @ref MyPostgresPool Only.
		 * 
		 */
		MyPostgres(pqxx::connection*, std::function<void()>);
		MyPostgres(const MyPostgres&) = delete;
		MyPostgres(MyPostgres&&) = delete;
		MyPostgres &operator=(const MyPostgres&) = delete;
		MyPostgres& operator=(MyPostgres&&) = delete;
		~MyPostgres();
		/**
		 * @brief Abort every query that has been executed before.
		 * 
		 */
		void abort();
		/**
		 * @brief Commit every query that has been executed before.
		 * 
		 */
		void commit();
		/**
		 * @brief Register a new Account and Get the Account ID Number.
		 * 
		 * @param email ID string
		 * @param pwd hashed password
		 * @return Account_ID_t New Account ID Number on Success. it'll be 0 on failure.
		 */
		Account_ID_t RegisterAccount(std::string email, Pwd_Hash_t pwd);
		/**
		 * @brief Find Account with ID and Password.
		 * 
		 * @param email ID string
		 * @param pwd hashed password
		 * @return Account_ID_t Account ID Number on Success. It'll be 0 on failure.
		 */
		Account_ID_t FindAccount(std::string email, Pwd_Hash_t pwd);
		/**
		 * @brief Change Account's Password.
		 * 
		 * @param id ID Number
		 * @param old_pwd Old Password
		 * @param new_pwd New Password
		 * @return true on Success.
		 * @return false on Failure.
		 */
		bool ChangePwd(Account_ID_t id, Pwd_Hash_t old_pwd, Pwd_Hash_t new_pwd);
		/**
		 * @brief Get all Information abount the Account.
		 * 
		 * @param account_id ID Number
		 * @return std::tuple<Achievement_ID_t, int, int, int> {Nickname, Win, Draw, Loose}
		 */
		std::tuple<Achievement_ID_t, int, int, int> GetInfo(Account_ID_t account_id);
		/**
		 * @brief Get all Achievement of the Account. if there are no Achievement ID, that means it's count number is 0.
		 * 
		 * @param account_id ID Number
		 * @return std::map<Achievement_ID_t, int> {Achievement ID Number, Achievement Count Number}
		 */
		std::map<Achievement_ID_t, int> GetAllAchieve(Account_ID_t account_id);
		/**
		 * @brief Set Nickname of the Account.
		 * 
		 * @param account_id ID Number
		 * @param achieve_id Achievement ID Number to Set Nickname
		 * @return true on success.
		 * @return false on failure.
		 */
		bool SetNickName(Account_ID_t account_id, Achievement_ID_t achieve_id);
		/**
		 * @brief Count 1 up on the Achievement of the Account and Get whether the achievement is fulfilled or not.
		 * This function should be called on 'Stacking' type of achievement.
		 * 
		 * @param id ID Number
		 * @param achieve Achievement ID Number to count up
		 * @return true on achieving.
		 * @return false on shortage or exceed.
		 */
		bool Achieve(Account_ID_t id, Achievement_ID_t achieve);
		/**
		 * @brief Update the progress of the Achievement on increasing and Get whether the achievement is fulfilled or not.
		 * This function should be called on 'Maximum Performing' type of achievement.
		 * 
		 * @param id ID Number
		 * @param achieve Achievement ID Number to update
		 * @param c progress amount of the achievement.
		 * @return true on achieving.
		 * @return false on shortage or exceed.
		 */
		bool AchieveProgress(Account_ID_t id, Achievement_ID_t achieve, int c);
		/**
		 * @brief Log the result of the battle.
		 * 
		 * @param winner Winner's ID Number
		 * @param looser Looser's ID Number
		 * @param draw whether draw or not.
		 * @param crashed whether crashed or not.
		 */
		void ArchiveBattle(Account_ID_t winner, Account_ID_t looser, bool draw, bool crashed);

	private:
		std::tuple<int, int> GetAchieve(Account_ID_t, Achievement_ID_t);
		Hash_t GetPwdHash(Account_ID_t, Pwd_Hash_t, std::string);
};