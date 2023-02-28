#pragma once
#include <stdexcept>
#include <string>
#include "Logger.hpp"

#define __STACKINFO__	__FILE__, __FUNCTION__, __LINE__

namespace mylib{
namespace utils{

class StackTraceExcept : public std::exception
{
	private:
		static const std::string CR;
		static const std::string TAB;
		std::string stacktrace;
	public:
		StackTraceExcept(std::string, std::string, std::string, int);
		StackTraceExcept(std::exception, std::string, std::string, int);
		void stack(std::string, std::string, int);
		const char * what() const noexcept override;
		void log();
};

}
}