#include <gtest/gtest.h>
#include <vector>
#include "MyMatchMaker.hpp"

#define INTERPOLATE 0.1

TEST(MatchMakerTest, BasicTest)
{
	MyMatchMaker mm(INTERPOLATE);
	std::vector<queue_element> vec({
		{1, 0, 0, 0},	//id는 1 이상이어야 한다.
		{2, 0, 0, 0},
		{3, 0, 0, 0},
		{4, 0, 0, 0}
	});

	std::random_shuffle(vec.begin(), vec.end());

	for(auto user : vec)
		mm.Enter(user);

	mm.Process();

	int match_count = 0;
	while(mm.isThereMatch())
	{
		auto [lp, rp] = mm.GetMatch();
		mm.PopMatch();
		ASSERT_NE(lp.whoami(), rp.whoami()) << "Same Account Matched";
		match_count++;
	}
	ASSERT_EQ(match_count, vec.size() / 2);
}

TEST(MatchMakerTest, FairTest)
{
	MyMatchMaker mm(INTERPOLATE);
	std::vector<queue_element> vec;
	for(int i = 0;i<100;i++)
		vec.push_back(queue_element(i + 1, i, 0, 100-i));

	std::random_shuffle(vec.begin(), vec.end());

	for(auto user : vec)
		mm.Enter(user);

	int match_count = 0;
	mm.Process();
	while(mm.isThereMatch())
	{
		auto [lp, rp] = mm.GetMatch();
		mm.PopMatch();
		ASSERT_LE(std::abs(lp.ratio() - rp.ratio()), 0.011) << lp.whoami() << " vs " << rp.whoami() << " -> Unfair";
		match_count++;
	}
	ASSERT_EQ(match_count, 100/2);
}

TEST(MatchMakerTest, OrderTest)
{
	MyMatchMaker mm(INTERPOLATE);
	std::vector<queue_element> vec({
		{1, 70, 0, 30},
		{2, 30, 0, 70},	//dummy
		{3, 20, 0, 80},	//dummy
		{4, 70, 0, 30},
		{5, 70, 0, 30}
	});

	Account_ID_t late_one = vec[vec.size() - 1].whoami();

	//std::random_shuffle(vec.begin(), vec.end());	//먼저 들어온 유저를 먼저 매칭한다.(조건이 같을 경우)

	for(auto user : vec)
		mm.Enter(user);

	mm.Process();
	while(mm.isThereMatch())
	{
		auto [lp, rp] = mm.GetMatch();
		mm.PopMatch();
		ASSERT_NE(lp.whoami(), late_one) << late_one << " vs " << rp.whoami();
		ASSERT_NE(rp.whoami(), late_one) << late_one << " vs " << lp.whoami();
	}
}

TEST(MatchMakerTest, RearrangeTest)
{
	MyMatchMaker mm(INTERPOLATE);
	std::vector<queue_element> vec({
		{1, 80, 0, 20},	//win rate 80%
		{2, 20, 0, 80},	//win rate 20%
	});

	int rearr_count = std::ceil(std::abs(vec[0].ratio() - vec[1].ratio()) / INTERPOLATE);

	for(auto user : vec)
		mm.Enter(user);

	int process_count = 0;

	while(!mm.isThereMatch())
	{
		mm.Process();
		process_count++;
	}
	ASSERT_LE(process_count, rearr_count);
}

TEST(MatchMakerTest, ExitTest)
{
	MyMatchMaker mm(INTERPOLATE);
	std::vector<queue_element> vec;
	for(int i = 0;i<100;i++)
		vec.push_back(queue_element(i + 1, i, 0, 100-i));

	std::random_shuffle(vec.begin(), vec.end());

	for(auto user : vec)
		mm.Enter(user);

	mm.Exit(3);
	mm.Exit(3);

	mm.Process();
	while(mm.isThereMatch())
	{
		auto [lp, rp] = mm.GetMatch();
		mm.PopMatch();
		ASSERT_NE(lp.whoami(), 3);
		ASSERT_NE(rp.whoami(), 3);
	}
}