#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

const std::string StackTraceExcept::CR = "\r\n";
const std::string StackTraceExcept::TAB = "\t";

StackTraceExcept::StackTraceExcept(std::string content, std::string file, std::string func, int line)
{
	stacktrace = "[MyException] " + content + CR + "[Stack Trace]" + CR;
	stack(file, func, line);
}

StackTraceExcept::StackTraceExcept(std::exception e, std::string file, std::string func, int line)
{
	stacktrace = "[Exception] " + std::string(e.what()) + CR + "[Stack Trace]" + CR;
	stack(file, func, line);
}

void StackTraceExcept::stack(std::string file, std::string func, int line)
{
	stacktrace += TAB + func + "(" + file + ":" + std::to_string(line) + ")" + CR;
}

const char * StackTraceExcept::what() const noexcept
{
	return stacktrace.c_str();
}

void StackTraceExcept::log()
{
	Logger::log(stacktrace, Logger::LogType::error);
}

}
}