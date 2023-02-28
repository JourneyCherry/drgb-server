#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

const std::string StackTraceExcept::CR = "\r\n";
const std::string StackTraceExcept::TAB = "\t";

StackTraceExcept::StackTraceExcept(std::string name, std::string content, std::string file, std::string func, int line)
{
	stacktrace = "[" + name + "] " + content + CR + "[Stack Trace]" + CR;
	stack(file, func, line);
}

StackTraceExcept::StackTraceExcept(std::string content, std::string file, std::string func, int line)
 : StackTraceExcept("Except", content, file, func, line) {}

void StackTraceExcept::stack(std::string file, std::string func, int line)
{
	stacktrace += TAB + func + "(" + file + ":" + std::to_string(line) + ")" + CR;
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

}
}