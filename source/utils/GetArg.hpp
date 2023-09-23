#pragma once
#include <getopt.h>
#include <string>
#include <functional>
#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

/**
 * @brief class to interpret argument string of program executor.
 * 
 */
class GetArg
{
	protected:
		/**
		 * @brief interpret option's argument as 'key=value' type.
		 * 
		 * @param input option's argument to interpret
		 * @param var result of 'key' as 'variable' name
		 * @param value result of 'value'
		 * @return true if it can be interpreted and successed.
		 * @return false if it cannot be interpreted or failed.
		 */
		bool splitarg(std::string input, std::string &var, std::string &value);

	public:
		virtual void ClearOpt() = 0;
		virtual void GetArgs(int, char**) = 0;	//This method is for global variable of 'GetArg' that cannot parse arguments in constructor.
};

}
}