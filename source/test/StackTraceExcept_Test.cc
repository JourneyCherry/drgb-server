#include <gtest/gtest.h>
#include <regex>
#include "StackTraceExcept.hpp"

using mylib::utils::StackTraceExcept;
using mylib::utils::StackErrorCode;
using mylib::utils::ErrorCodeExcept;

StackErrorCode func()
{
	return {ERR_PROTOCOL_VIOLATION, __STACKINFO__};
}

const char *expr = "\\[.*\\] .*\r\n\\[Stack Trace\\]\r\n(\t.*\\(.*\\)\r\n)*";

TEST(StackTraceExceptTest, StackTraceTest)
{
	try
	{
		try
		{
			throw StackTraceExcept("Hello World!", __STACKINFO__);
		}
		catch(StackTraceExcept e)
		{
			e.Propagate(__STACKINFO__);
		}
	}
	catch(const StackTraceExcept &e)
	{
		std::regex re(expr);
		EXPECT_TRUE(std::regex_match(e.what(), re)) << e.what();
	}
}

TEST(StackTraceExceptTest, ErrorCodeTest)
{
	try
	{
		ErrorCodeExcept::ThrowOnFail(func(), __STACKINFO__);
	}
	catch(const std::exception& e)
	{
		std::regex re(expr);
		EXPECT_TRUE(std::regex_match(e.what(), re)) << e.what();
	}
}