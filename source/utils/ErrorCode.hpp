#pragma once
#include <map>
#include <string>
#include <cerrno>
#include <boost/asio/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <openssl/err.h>
#include "MyCodes.hpp"

namespace mylib{
namespace utils{

/**
 * @brief Integration of Error Code and Message types.
 * 
 */
class ErrorCode
{
	private:
		static const std::map<errorcode_t, std::string> strerrcode;
		
	private:
		int m_code;
		int m_type;
		std::string m_type_str;
		std::string m_message;

	public:
		static constexpr int TYPE_ERRNO = 0;
		static constexpr int TYPE_BOOST = 1;
		static constexpr int TYPE_OPENSSL = 2;
		static constexpr int TYPE_CUSTOM = 3;

		/**
		 * @brief Create new ErrorCode object with default code(SUCCESS, CustomCode).
		 * 
		 */
		ErrorCode();
		/**
		 * @brief Create new ErrorCode object with Boost's system error code.
		 * 
		 * @param ec Error Code
		 */
		ErrorCode(boost::system::error_code ec);
		/**
		 * @brief Create new ErrorCode object with POSIX-Compatible system error code(errno).
		 * 
		 * @param _errno errno.
		 */
		ErrorCode(int _errno);
		/**
		 * @brief Create new ErrorCode object with Custom error code.
		 * @see MyCodes.hpp
		 * 
		 * @param ec Custom Error Code
		 */
		ErrorCode(errorcode_t ec);
		/**
		 * @brief Create new ErrorCode object with OpenSSL error code.
		 * 
		 * @param ec Error Code
		 */
		ErrorCode(unsigned long ec);
		ErrorCode(const ErrorCode&);
		/**
		 * @brief Set new error message.
		 * 
		 * @param new_message new error message
		 */
		void SetMessage(std::string new_message);
		/**
		 * @brief Get Error Code Number.
		 * 
		 * @return int Error Code
		 */
		int code() const;
		/**
		 * @brief Get Error Message
		 * 
		 * @return std::string Error Message
		 */
		std::string message() const;
		/**
		 * @brief Get Error Message with Error Code
		 * 
		 * @return std::string Error Message
		 */
		std::string message_code() const;
		/**
		 * @brief Get Type of ErrorCode in string
		 * 
		 * @return std::string type
		 */
		std::string typestr() const;
		/**
		 * @brief Get Type of ErrorCode.
		 * 
		 * @return int type.
		 * @param 0 for posix
		 * @param 1 for boost
		 * @param 2 for openssl
		 * @param 3 for custom
		 */
		int typecode() const;
		ErrorCode &operator=(const ErrorCode&);
		operator bool() const;
		/**
		 * @brief Get Success or not.
		 * 
		 * @return true if successed.
		 * @return false if failed.
		 */
		bool isSuccessed() const;
};

}
}