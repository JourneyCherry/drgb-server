#include <gtest/gtest.h>
#include "ConfigParser.hpp"

using mylib::utils::ConfigParser;

TEST(ConfigParserTest, ArgumentTest)
{
	//Test Executable이 생성되는 위치가 drgb_server/source/build/test 이다.
	ASSERT_TRUE(ConfigParser::ReadFile("../../test/ConfigParser_Test_Resource.conf"));

	//Default Type Test
	EXPECT_EQ(3, ConfigParser::GetInt("integer"));
	EXPECT_DOUBLE_EQ(3.1415926535, ConfigParser::GetDouble("floating"));
	EXPECT_STREQ("pi", ConfigParser::GetString("string").c_str());
	EXPECT_STREQ("", ConfigParser::GetString("Commentary").c_str());

	//Integer Test
	EXPECT_EQ(0, ConfigParser::GetInt("overflow_integer"));

	//Double Test
	EXPECT_DOUBLE_EQ(314159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848111745028410270193852110555964462294895493038196.4428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132, ConfigParser::GetDouble("overflow_double"));

	//String Test
	EXPECT_STREQ("hello another world!", ConfigParser::GetString("space").c_str());
	EXPECT_STREQ("hello another world!", ConfigParser::GetString("quote_space").c_str());
	EXPECT_STREQ("hello", ConfigParser::GetString("double_quote").c_str());
	EXPECT_STREQ("hi other world!", ConfigParser::GetString("name space").c_str());
	EXPECT_STREQ("value quote", ConfigParser::GetString("\"name quote\"").c_str());
	EXPECT_STREQ("", ConfigParser::GetString("multiple").c_str());
	EXPECT_STREQ("do not trim here   			   ", ConfigParser::GetString("untrimmed").c_str());
	EXPECT_STREQ("", ConfigParser::GetString("unclosed quote").c_str());
}