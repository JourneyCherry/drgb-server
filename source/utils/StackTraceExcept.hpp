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
		std::string exceptname;

	protected:
		StackTraceExcept() = default;
		StackTraceExcept(std::string, std::string, std::string, std::string, int);

	public:
		StackTraceExcept(std::string, std::string, std::string, int);
		void stack(std::string, std::string, int);
		void Propagate(std::string, std::string, int);
		const char * what() const noexcept override;
};

class ErrorCodeExcept : public StackTraceExcept
{
	public:
		ErrorCodeExcept(ErrorCode ec, std::string file, std::string func, int line) : StackTraceExcept(ec.typestr(), ec.message_code(), file, func, line) {}
};

}
}