#pragma once
#include <getopt.h>
#include <string>
#include <functional>
#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

class GetArg
{
	protected:
		bool splitarg(std::string, std::string&, std::string&);

	public:
		virtual void ClearOpt() = 0;
		virtual void GetArgs(int, char**) = 0;
};

}
}