#include <gtest/gtest.h>
#include "ErrorCode.hpp"

using mylib::utils::ErrorCode;

class ErrorCodeTest : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			custom = ErrorCode(ERR_PROTOCOL_VIOLATION);
			system = ErrorCode(EINVAL);
			boost = ErrorCode(boost::beast::error::timeout);
		}

	ErrorCode custom;
	ErrorCode system;
	ErrorCode boost;
};

TEST_F(ErrorCodeTest, InputTest)
{
	ErrorCode myCode = custom;
	EXPECT_EQ(custom.code(), myCode.code());
	EXPECT_EQ(custom.typecode(), myCode.typecode());
}

TEST_F(ErrorCodeTest, MessageTest)
{
	EXPECT_STREQ("Protocol Violation", custom.message().c_str());
	EXPECT_STREQ("Invalid argument", system.message().c_str());
	EXPECT_STREQ("The socket was closed due to a timeout", boost.message().c_str());
	custom.SetMessage("Custom Message");
	EXPECT_STREQ("(Custom Message)", custom.message().c_str());
}