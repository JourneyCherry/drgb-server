#pragma once
#include <mutex>
#include <hiredis/hiredis.h>
#include "MyRedisReply.hpp"
#include "ErrorCode.hpp"
#include "Expected.hpp"
#include "Encoder.hpp"

using mylib::utils::ErrorCode;
using mylib::utils::Expected;
using mylib::utils::Encoder;

/**
 * @brief Cusomized Redis Client Library.
 * 
 */
class MyRedis
{
	private:
		redisContext *ctx;
		std::mutex mtx;
		int sessionTimeout;
		std::string timeoutStr;

	protected:
		/**
		 * @brief Get Custom ErrorCode from Redis Context. It should be used when the Connection has a problem.
		 * 
		 * @return ErrorCode derived from redisContext
		 */
		ErrorCode GetErrorCode();
		/**
		 * @brief Make Custom ErrorCode object.
		 * 
		 * @param code error code
		 * @param msg customized error message
		 * @return ErrorCode that is Customized.
		 */
		static ErrorCode GetErrorCode(int code, std::string msg);
		std::vector<MyRedisReply> Query(std::initializer_list<std::string>);
		std::vector<MyRedisReply> Query(std::vector<std::string>);

	public:
		/**
		 * @brief Construct a new MyRedis object.
		 * 
		 * @param timeout Session Timeout Duration(ms).
		 */
		MyRedis(int timeout) : ctx(nullptr), sessionTimeout(timeout), timeoutStr(std::to_string(sessionTimeout)) {}
		~MyRedis();
		/**
		 * @brief Connect to Redis Server.
		 * 
		 * @param addr Address of Server
		 * @param port Port of Server
		 * @return ErrorCode - Result of Connection.
		 */
		ErrorCode Connect(std::string addr, int port);
		/**
		 * @brief Close Connection.
		 * 
		 */
		void Close();
		/**
		 * @brief Check if the redis server is connected or not.
		 * 
		 * @return true if the server is connected.
		 * @return false if the server is disconnected.
		 */
		bool is_open() const;

		/**
		 * @brief Write informations of the Client Session to Redis
		 * 
		 * @param id Account ID of User
		 * @param cookie Cookie value of the Session
		 * @param serverName Server Location where the Client is connected. This value represents the name of Server.
		 * @return ErrorCode - Result of Command.
		 */
		ErrorCode SetInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName);
		/**
		 * @brief Refresh expiration time 
		 * 
		 * @param id 
		 * @param cookie 
		 * @param serverName 
		 * @return true 
		 * @return false 
		 */
		bool RefreshInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName);
		/**
		 * @brief Get informations of the session From Account ID.
		 * 
		 * @param id Account ID of User
		 * @return Result of command.
		 * @return First value is cookie.
		 * @return Second value is Server name.
		 * @return ErrorCode is for when it Failed.
		 */
		Expected<std::pair<Hash_t, std::string>, ErrorCode> GetInfoFromID(const Account_ID_t& id);
		/**
		 * @brief Get informations of the session From Cookie.
		 * 
		 * @param cookie Cookie of the session.
		 * @return Result of command.
		 * @return First value is Account ID.
		 * @return Second value is Server name.
		 * @return ErrorCode is for when it Failed.
		 */
		Expected<std::pair<Account_ID_t, std::string>, ErrorCode> GetInfoFromCookie(const Hash_t& cookie);
		/**
		 * @brief Remove informations of the Client Session from Redis. It works with precise informations only for data integrity.
		 * 
		 * @param id Account ID of User
		 * @param cookie Cookie value of the Session
		 * @param serverName Server Location where the Client is connected. This value represents the name of Server.
		 * @return true if the informations is deleted successfully.
		 * @return false if the informations isn't deleted because of mismatch.
		 */
		bool ClearInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName);
};