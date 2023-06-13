#include <gtest/gtest.h>
#include "Expected.hpp"

using mylib::utils::Expected;

TEST(ExpectedTest, ConstructTest)
{
	Expected<std::string, int> success("Hello World");
	Expected<std::string, int> fail(5);
	Expected<std::string, int> both("Hi Another World", 9, false);
	EXPECT_TRUE((bool)success);
	EXPECT_FALSE((bool)fail);
	EXPECT_FALSE((bool)both);
	EXPECT_STREQ("Hello World", success->c_str());
	EXPECT_EQ(5, fail.error());
	EXPECT_STREQ("Hi Another World", both->c_str());
	EXPECT_EQ(9, both.error());

	EXPECT_STREQ("default", fail.value_or("default").c_str());
}

TEST(ExpectedTest, CopyMoveTest)
{
	Expected<std::string, int> origin("Original");
	Expected<std::string, int> copy(3);
	Expected<std::string, int> move(5);
	EXPECT_FALSE((bool)copy);
	EXPECT_FALSE((bool)move);

	copy = origin;
	move = std::move(origin);
	EXPECT_TRUE((bool)copy);
	EXPECT_TRUE((bool)move);
	EXPECT_STREQ("Original", copy->c_str());
	EXPECT_STREQ("Original", move->c_str());
}

TEST(ExpectedTest, ComparisonTest)
{
	Expected<std::string, int> successA("A", 3, true);
	Expected<std::string, int> successB("B", 5, true);
	Expected<std::string, int> failA("A", 3, false);
	Expected<std::string, int> failB("B", 5, false);

	Expected<std::string, int> comparison("A");
	EXPECT_TRUE(comparison == successA);
	EXPECT_FALSE(comparison == successB);
	EXPECT_FALSE(comparison == failA);
	comparison = Expected<std::string, int>("A", 5, false);
	EXPECT_FALSE(comparison == failA);
	EXPECT_TRUE(comparison == failB);
}

TEST(ExpectedTest, SingleTypeTest)
{
	Expected<int> success(5);
	Expected<int> fail;

	EXPECT_EQ(3, fail.value_or(3));
	EXPECT_FALSE(success == fail);
	fail = success;
	EXPECT_EQ(5, *fail);
}