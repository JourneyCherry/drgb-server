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
		StackTraceExcept(std::string, std::string, std::string, int);
		void stack(std::string, std::string, int);
		void Propagate(std::string, std::string, int);
		const char * what() const noexcept override;
};

class StackErrorCode : public ErrorCode
{
	private:
		static const std::string CR;
		static const std::string TAB;
		std::string stacktrace;
	
	public:
		StackErrorCode() : stacktrace(), ErrorCode() {}
		StackErrorCode(std::string fi, std::string fu, int l) : ErrorCode() { stack(fi, fu, l); }
		StackErrorCode(boost::system::error_code ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		StackErrorCode(int ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		StackErrorCode(errorcode_t ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		StackErrorCode(unsigned long ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		StackErrorCode(ErrorCode ec, std::string fi, std::string fu, int l) : ErrorCode(ec) { stack(fi, fu, l); }
		StackErrorCode(StackErrorCode sec, std::string fi, std::string fu, int l) : ErrorCode(sec) { stacktrace = sec.stacktrace; stack(fi, fu, l); }
		StackErrorCode(const StackErrorCode& copy) { (*this) = copy; }
		StackErrorCode &operator=(const StackErrorCode&);
		std::string what() const;
	
	private:
		void stack(std::string, std::string, int);

	friend class ErrorCodeExcept;
};

class ErrorCodeExcept : public StackTraceExcept
{
	public:
		ErrorCodeExcept(ErrorCode ec, std::string file, std::string func, int line) : StackTraceExcept(ec.typestr(), ec.message_code(), file, func, line) {}
		ErrorCodeExcept(StackErrorCode sec) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.stacktrace) {}
		ErrorCodeExcept(StackErrorCode sec, std::string file, std::string func, int line) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.stacktrace)
		{
			stack(file, func, line);
		}
		static void ThrowOnFail(ErrorCode, std::string, std::string, int);
		static void ThrowOnFail(StackErrorCode, std::string, std::string, int);
};

}
}