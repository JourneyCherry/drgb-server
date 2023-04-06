#pragma once
#include <stdexcept>
#include <string>
#include <boost/beast/core.hpp>
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
		StackTraceExcept(std::string, std::string, std::string, std::string, int);

	public:
		StackTraceExcept(std::string, std::string, std::string, int);
		void stack(std::string, std::string, int);
		void Propagate(std::string, std::string, int);
		const char * what() const noexcept override;
};

class StackErrorCode : public ErrorCode
{
	private:
		std::string file;
		std::string func;
		int line;
	
	public:
		StackErrorCode() : file(), func(), line(-1), ErrorCode() {}
		StackErrorCode(std::string fi, std::string fu, int l) : file(fi), func(fu), line(l), ErrorCode() {}
		StackErrorCode(boost::system::error_code ec, std::string fi, std::string fu, int l) : file(fi), func(fu), line(l), ErrorCode(ec) {}
		StackErrorCode(int ec, std::string fi, std::string fu, int l) : file(fi), func(fu), line(l), ErrorCode(ec) {}
		StackErrorCode(errorcode_t ec, std::string fi, std::string fu, int l) : file(fi), func(fu), line(l), ErrorCode(ec) {}
		StackErrorCode(ErrorCode ec, std::string fi, std::string fu, int l) : file(fi), func(fu), line(l), ErrorCode(ec) {}
		StackErrorCode(const StackErrorCode& copy) { (*this) = copy; }
		StackErrorCode &operator=(const StackErrorCode& copy)
		{
			ErrorCode::operator=(copy);
			file = copy.file;
			func = copy.func;
			line = copy.line;

			return *this;
		}
		std::string message_code_stack() { return message_code() + "(" + func + " : " + file + ":" + std::to_string(line) + ")"; }
		bool isStacked(){ return (line >= 0); }

	friend class ErrorCodeExcept;
};

class ErrorCodeExcept : public StackTraceExcept
{
	public:
		ErrorCodeExcept(ErrorCode ec, std::string file, std::string func, int line) : StackTraceExcept(ec.typestr(), ec.message_code(), file, func, line) {}
		ErrorCodeExcept(StackErrorCode sec) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.file, sec.func, sec.line) {}
		ErrorCodeExcept(StackErrorCode sec, std::string file, std::string func, int line) : StackTraceExcept(sec.typestr(), sec.message_code(), sec.isStacked()?sec.file:file, sec.isStacked()?sec.func:func, sec.isStacked()?sec.line:line)
		{
			if(sec.isStacked())
				stack(file, func, line);
		}
};

}
}