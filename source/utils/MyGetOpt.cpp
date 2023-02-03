#include "MyGetOpt.hpp"

bool MyCommon::MyGetOpt::splitarg(std::string input, std::string &var, std::string &value)
{
	int ptr = input.find('=');
	if(ptr <= 0 || ptr == std::string::npos || ptr >= input.length())
		return false;

	var = input.substr(0, ptr);
	value = input.substr(ptr + 1);

	return true;
}