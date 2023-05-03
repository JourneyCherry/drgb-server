#include <gtest/gtest.h>
#include "DeMap.hpp"

using mylib::utils::DeMap;

class DeMapTest : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			for(int i = 0;i<10;i++)
				map.Insert(i, i*2, i*3);
		}

		void TearDown() override
		{
			map.Clear();
		}

	DeMap<int, int, int> map;
};

TEST_F(DeMapTest, SearchTest)
{
	for(int i = 0;i<10;i++)
	{
		auto resultL = map.FindLKey(i);
		ASSERT_TRUE(resultL.isSuccessed());
		EXPECT_EQ(i*2, resultL->first);
		EXPECT_EQ(i*3, resultL->second);

		auto resultR = map.FindRKey(i*2);
		ASSERT_TRUE(resultR.isSuccessed());
		EXPECT_EQ(i, resultR->first);
		EXPECT_EQ(i*3, resultR->second);
	}
}


TEST_F(DeMapTest, InOutTest)
{
	EXPECT_TRUE(map.Insert(12, 24, 36));
	EXPECT_FALSE(map.Insert(0, 100, 200)) << "Left Key Duplicated";
	EXPECT_FALSE(map.Insert(100, 0, 200)) << "Right Key Duplicated";
	auto one = map.EraseLKey(1);
	EXPECT_TRUE(one.isSuccessed());
	EXPECT_EQ(std::get<0>(*one), 1);
	EXPECT_EQ(std::get<1>(*one), 2);
	EXPECT_EQ(std::get<2>(*one), 3);

	auto two = map.EraseRKey(4);
	EXPECT_TRUE(two.isSuccessed());
	EXPECT_EQ(std::get<0>(*two), 2);
	EXPECT_EQ(std::get<1>(*two), 4);
	EXPECT_EQ(std::get<2>(*two), 6);

	EXPECT_FALSE(map.EraseLKey(1000));
	EXPECT_FALSE(map.EraseRKey(1000));

	EXPECT_EQ(map.Size(), 9);
	map.Clear();
	EXPECT_EQ(map.Size(), 0);
}

TEST_F(DeMapTest, ModTest)
{
	EXPECT_EQ(0, map.FindLKey(0)->second);
	EXPECT_TRUE(map.InsertLKeyValue(0, 100));
	EXPECT_EQ(100, map.FindRKey(0)->second);
	
	EXPECT_FALSE(map.InsertLKeyValue(100, 200)) << "Modify Uninserted Data by Left Key";
	EXPECT_FALSE(map.InsertRKeyValue(100, 200)) << "Modify Uninserted Data by Right Key";
}