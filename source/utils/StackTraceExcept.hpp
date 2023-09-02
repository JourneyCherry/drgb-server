#pragma once
#include <stdexcept>
#include <string>
#include <boost/beast/core.hpp>
#include <openssl/err.h>
#include <cerrno>
#include "ErrorCode.hpp"

#define __STACKINFO__	__FILE__, __FUNCTION__, __LINE__

namespace mylib{
namespace utils{

/**
 * @brief Exception holding StackTrace Message.
 * 
 */
class StackTraceExcept : public std::exception
{
	private:
		static const std::string CR;
		static const std::string TAB;
		std::string stacktrace;

	protected:
		StackTraceExcept() = default;
		StackTraceExcept(std::string, std::string, std::string);
		StackTraceExcept(std::string, std::string, std::string, std::string, int);
		void stack(std::string);

	public:
		/**
		 * @brief Constructor. use like 'StackTraceExcept("Message", __STACKINFO__)'
		 * 
		 */
		StackTraceExcept(std::string, std::string, std::string, int);
		/**
		 * @brief stack this call stack. use like 'stack(__STACKINFO__)'.
		 * 
		 */
		void stack(std::string, std::string, int);
		/**
		 * @brief Stack this call stack and throw itself. use like 'Propagate(__STACKINFO__)'.
		 * 
		 * @throw StackTraceExcept itself.
		 * 
		 */
		void Propagate(std::string, std::string, int);
		const char * what() const noexcept override;
};

/**
 * @brief ErrorCode holding StackTrace Message.
 * 
 */
class StackErrorCode : public ErrorCode
{
	private:
		static const std::string CR;
		static const std::string TAB;
		std::string stacktrace;
	
	public:
		StackErrorCode() : stacktrace(), ErrorCode() {}
		/**
		 * @brief Constructor of Success. use like 'StackErrorCode(__STACKINFO__)'.
		 * 
		 */
		StackErrorCode(std::string fi, std::string fu, int l) : ErrorCode() { stack(fi, fu, l); }
		/**
		 * @brief Constructor of Boost ErrorCode. use like 'StackErrorCode(ec, __STACKINFO__)'.
		 * 
		 * @param ec Boost system error code.
		 */
		StackErrorCode(boost::system::error_code ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		/**
		 * @brief Constructor of POSIX System ErrorCode. use like 'StackErrorCode(ec, __STACKINFO__)'.
		 * 
		 * @param ec POSIX System error code.
		 */
		StackErrorCode(int ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		/**
		 * @brief Constructor of Custom ErrorCode. use like 'StackErrorCode(ec, __STACKINFO__)'.
		 * 
		 * @param ec Custom error code.
		 */
		StackErrorCode(errorcode_t ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		/**
		 * @brief Constructor of OpenSSL ErrorCode. use like 'StackErrorCode(ec, __STACKINFO__)'.
		 * 
		 * @param ec OpenSSL error code.
		 */
		StackErrorCode(unsigned long ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		/**
		 * @brief Constructor of ErrorCode. use like 'StackErrorCode(ec, __STACKINFO__)'.
		 * 
		 * @param ec ErrorCode
		 */
		StackErrorCode(ErrorCode ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		/**
		 * @brief Constructor stacking on StackErrorCode. use like 'StackErrorCode(sec, __STACKINFO__)'
		 * 
		 * @param sec StackErrorCode.
		 */
		StackErrorCode(StackErrorCode sec, std::string fi, std::string fu, int l) : ErrorCode(sec) { stacktrace = sec.stacktrace; stack(fi, fu, l); }
		StackErrorCode(const StackErrorCode& copy) { (*this) = copy; }
		StackErrorCode &operator=(const StackErrorCode&);
		std::string what() const;
	
	private:
		void stack(std::string, std::string, int);

	friend class ErrorCodeExcept;
};

/**
 * @brief StackTraceExcept holding ErrorCode.
 * 
 */
class ErrorCodeExcept : public StackTraceExcept
{
	public:
		/**
		 * @brief Constructor of ErrorCode. use like 'throw ErrorCodeExcept(ec, __STACKINFO__)'.
		 * 
		 * @param ec ErrorCode
		 */
		ErrorCodeExcept(ErrorCode ec, std::string file, std::string func, int line) : StackTraceExcept(ec.typestr(), ec.message_code(), file, func, line) {}
		/**
		 * @brief Constructor of StackErrorCode without stacking. use like 'throw ErrorCodeExcept(sec)'.
		 * 
		 * @param sec StackErrorCode
		 */
		ErrorCodeExcept(StackErrorCode sec) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.stacktrace) {}
		/**
		 * @brief Constructor of StackErrorCode with stacking. use like 'throw ErrorCodeExcept(sec, __STACKINFO__)'.
		 * 
		 * @param sec StackErrorCode
		 */
		ErrorCodeExcept(StackErrorCode sec, std::string file, std::string func, int line) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.stacktrace)
		{
			stack(file, func, line);
		}
		/**
		 * @brief Rethrow if the ErrorCode has failed. use like 'ThrowOnFail(ec, __STACKINFO__)'.
		 * 
		 * @param ec ErrorCode
		 */
		static void ThrowOnFail(ErrorCode ec, std::string, std::string, int);
		/**
		 * @brief Rethrow if the StackErrorCode has failed. use like 'ThrowOnFail(sec, __STACKINFO__)'.
		 * 
		 * @param sec StackErrorCode
		 */
		static void ThrowOnFail(StackErrorCode sec, std::string, std::string, int);
};

}
}