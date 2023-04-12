#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

const std::string StackTraceExcept::CR = "\r\n";
const std::string StackTraceExcept::TAB = "\t";
const std::string StackErrorCode::CR = "\r\n";
const std::string StackErrorCode::TAB = "\t";

StackTraceExcept::StackTraceExcept(std::string name, std::string content, std::string stacktrace_str)
{
	stacktrace = "[" + name + "] " + content + CR + "[Stack Trace]" + CR;
	stack(stacktrace_str);
}

StackTraceExcept::StackTraceExcept(std::string name, std::string content, std::string file, std::string func, int line) : StackTraceExcept(name, content, "")
{
	stack(file, func, line);
}

StackTraceExcept::StackTraceExcept(std::string content, std::string file, std::string func, int line)
 : StackTraceExcept("Except", content, file, func, line) {}

void StackTraceExcept::stack(std::string file, std::string func, int line)
{
	stacktrace += TAB + func + "(" + file + ":" + std::to_string(line) + ")" + CR;
}

void StackTraceExcept::stack(std::string trace)
{
	stacktrace += trace;
}

void StackTraceExcept::Propagate(std::string file, std::string func, int line)
{
	stack(file, func, line);
	throw *this;
}

const char * StackTraceExcept::what() const noexcept
{
	return stacktrace.c_str();
}

void StackErrorCode::stack(std::string file, std::string func, int line)
{
	stacktrace += TAB + func + "(" + file + ":" + std::to_string(line) + ")" + CR;
}

StackErrorCode &StackErrorCode::operator=(const StackErrorCode& copy)
{
	ErrorCode::operator=(copy);
	stacktrace = copy.stacktrace;

	return *this;
}

void ErrorCodeExcept::ThrowOnFail(ErrorCode ec, std::string file, std::string func, int line)
{
	if(!ec)
		throw ErrorCodeExcept(ec, file, func, line);
}

void ErrorCodeExcept::ThrowOnFail(StackErrorCode sec, std::string file, std::string func, int line)
{
	if(!sec)
		throw ErrorCodeExcept(sec, file, func, line);
}

}
}