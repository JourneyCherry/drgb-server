#include <gtest/gtest.h>
#include "MyGame.hpp"

TEST(GameTest, NormalTest)
{
	MyGame game;
	std::array<std::queue<int>, 2> actions;
	actions[0].push(MyGame::MEDITATE);
	actions[0].push(MyGame::MEDITATE);
	actions[0].push(MyGame::GUARD);
	actions[0].push(MyGame::FIRE);
	actions[1].push(MyGame::MEDITATE);
	actions[1].push(MyGame::GUARD);
	actions[1].push(MyGame::PUNCH);
	actions[1].push(MyGame::MEDITATE);
	std::vector<std::array<byte, 2>> answer_action({
		{MyGame::MEDITATE, MyGame::MEDITATE},
		{MyGame::MEDITATE, MyGame::GUARD},
		{MyGame::GUARD, MyGame::PUNCH},
		{MyGame::FIRE, MyGame::MEDITATE}
	});
	std::vector<std::array<int,2>> answer_health({
		{1, 1},
		{1, 1},
		{1, 1},
		{1, 0},
	});
	std::vector<std::array<int,2>> answer_energy({
		{1, 1},
		{2, 1},
		{2, 0},
		{0, 1}
	});

	for(int i = 0;i<MyGame::MAX_PLAYER;i++)
	{
		if(!actions[i].empty())
		{
			game.Action(i, actions[i].front());
			actions[i].pop();
		}
	}
	while(!game.process())
	{
		int round = game.GetRound();
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
		{
			auto [action, health, energy] = game.GetPlayerInfo(i);
			ASSERT_EQ(answer_action[round][i], action) << "Side : " << i << ", Round : " << round;
			ASSERT_EQ(answer_health[round][i], health) << "Side : " << i << ", Round : " << round;
			ASSERT_EQ(answer_energy[round][i], energy) << "Side : " << i << ", Round : " << round;
		}
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
		{
			if(!actions[i].empty())
			{
				game.Action(i, actions[i].front());
				actions[i].pop();
			}
		}
	}
	int round = game.GetRound();
	ASSERT_EQ(round, 3);
	int side = game.GetWinner();
	ASSERT_EQ(side, 0);
}

TEST(GameTest, RoundOverTest)
{
	auto test = [](std::vector<std::array<byte, 2>> actions) -> void
	{
		MyGame game;
		do
		{
			if(actions.size() > 0)
			{
				auto action = actions.front();
				game.Action(0, action[0]);
				game.Action(1, action[1]);
				actions.erase(actions.begin());
				actions.push_back(action);
			}
		}
		while(!game.process());
		ASSERT_GE(game.GetRound(), MyGame::MAX_ROUND);
		ASSERT_LT(game.GetWinner(), 0);
	};

	std::vector<std::array<byte, 2>> actions;

	test(actions);

	actions.push_back({MyGame::MEDITATE, MyGame::MEDITATE});
	actions.push_back({MyGame::PUNCH, MyGame::GUARD});
	actions.push_back({MyGame::GUARD, MyGame::PUNCH});
	actions.push_back({MyGame::MEDITATE, MyGame::MEDITATE});
	actions.push_back({MyGame::MEDITATE, MyGame::MEDITATE});
	actions.push_back({MyGame::FIRE, MyGame::EVADE});
	actions.push_back({MyGame::MEDITATE, MyGame::MEDITATE});
	actions.push_back({MyGame::EVADE, MyGame::FIRE});
	test(actions);
}

TEST(GameTest, CrashTest)
{
	MyGame game;
	do
	{
		int round = game.GetRound();
		ASSERT_LT(round, MyGame::MAX_ROUND);
		int winner = game.GetWinner();
		ASSERT_LT(winner, 0);
	}
	while(!game.process());
	int round = game.GetRound();
	ASSERT_EQ(round, MyGame::MAX_ROUND);
	int winner = game.GetWinner();
	ASSERT_LT(winner, 0);
}

TEST(GameTest, ActionLimitTest)
{
	auto EnergyTest = [](byte action)
	{
		MyGame game;
		do
		{
			for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			{
				auto [prev, health, energy] = game.GetPlayerInfo(i);
				if(prev == action)
					ASSERT_LE(energy, 0);
				game.Action(i, action);
			}
		}
		while(!game.process());
	};

	std::vector<byte> actions = {MyGame::EVADE, MyGame::PUNCH, MyGame::FIRE};
	for(byte act : actions)
		EnergyTest(act);

	auto NullTest = [](byte action)
	{
		MyGame game;
		bool acted = true;
		do
		{
			if(acted)
			{
				acted = false;
			}
			else
			{
				for(int i = 0;i<MyGame::MAX_PLAYER;i++)
				{
					auto [prev, health, energy] = game.GetPlayerInfo(i);
					ASSERT_NE(prev, action);
					game.Action(i, action);
				}
				acted = true;
			}
		}
		while(!game.process());
	};

	actions = {MyGame::GUARD, MyGame::EVADE, MyGame::PUNCH, MyGame::FIRE};
	for(byte act : actions)
		NullTest(act);
}

TEST(GameTest, AchievementTest)
{
	std::array<std::queue<byte>, 2> actions;
	actions[0] = std::queue<byte>({
		MyGame::MEDITATE,
		MyGame::MEDITATE,
		//GUARD ADDICTION
		MyGame::GUARD,
		MyGame::GUARD,
		//MEDITATE ADDICTION
		MyGame::MEDITATE,
		//Doppelganger
		MyGame::PUNCH,
		MyGame::MEDITATE,
		//EVADE ADDICTION
		MyGame::EVADE,
		//KneelInMySight
		MyGame::PUNCH
	});
	actions[1] = std::queue<byte>({
		MyGame::MEDITATE,
		MyGame::MEDITATE,
		//PUNCH ADDICTION
		MyGame::PUNCH,
		MyGame::PUNCH,
		MyGame::MEDITATE,
		//Doppelganger
		MyGame::PUNCH,
		MyGame::MEDITATE,
		MyGame::MEDITATE,
		MyGame::FIRE
	});

	std::map<byte, int> achieves;
	MyGame game;
	
	int log_count = 0;
	auto log_achievement = [&]() -> void
	{
		auto achievement = game.GetAchievement();
		while(achievement)
		{
			auto [side, achieve, count] = *achievement;
			switch(achieve)
			{
				//(행위의)최대 횟수만 카운팅 하는 업적
				case ACHIEVE_MEDITATE_ADDICTION:
				case ACHIEVE_GUARD_ADDICTION:
				case ACHIEVE_EVADE_ADDICTION:
				case ACHIEVE_PUNCH_ADDICTION:
				case ACHIEVE_FIRE_ADDICTION:
					if(achieves.find(achieve) == achieves.end())
						achieves.insert({achieve, count});
					else
						achieves[achieve] = count;
					break;
				//매 회마다 횟수가 1씩 증가하는 업적
				case ACHIEVE_NEWBIE:
				case ACHIEVE_NOIVE:
				case ACHIEVE_CHALLENGER:
				case ACHIEVE_DOMINATOR:
				case ACHIEVE_SLAYER:
				case ACHIEVE_CONQUERER:
				case ACHIEVE_KNEELINMYSIGHT:
				case ACHIEVE_DOPPELGANGER:
				case ACHIEVE_AREYAWINNINGSON:
					if(achieves.find(achieve) == achieves.end())
						achieves.insert({achieve, 1});
					else
						achieves[achieve] += count;
					break;
			}
			achievement = game.GetAchievement();
			log_count++;
		}
	};

	do
	{
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
		{
			if(actions[i].empty())
				continue;
			game.Action(i, actions[i].front());
			actions[i].pop();
		}
		log_achievement();
	}
	while(!game.process());
	ASSERT_GT(log_count, 0) << "No Achievement Among Game";
	log_achievement();

	EXPECT_EQ(achieves[ACHIEVE_MEDITATE_ADDICTION], 1);
	EXPECT_EQ(achieves[ACHIEVE_GUARD_ADDICTION], 2);
	EXPECT_EQ(achieves[ACHIEVE_EVADE_ADDICTION], 1);
	EXPECT_EQ(achieves[ACHIEVE_PUNCH_ADDICTION], 1);
	EXPECT_EQ(achieves[ACHIEVE_FIRE_ADDICTION], 1);
	EXPECT_EQ(achieves[ACHIEVE_DOPPELGANGER], 2);
	EXPECT_EQ(achieves[ACHIEVE_KNEELINMYSIGHT], 1);
	//이하 업적은 
	EXPECT_EQ(achieves[ACHIEVE_NEWBIE], 0);
	EXPECT_EQ(achieves[ACHIEVE_NOIVE], 0);
	EXPECT_EQ(achieves[ACHIEVE_CHALLENGER], 0);
	EXPECT_EQ(achieves[ACHIEVE_DOMINATOR], 0);
	EXPECT_EQ(achieves[ACHIEVE_SLAYER], 0);
	EXPECT_EQ(achieves[ACHIEVE_CONQUERER], 0);
	EXPECT_EQ(achieves[ACHIEVE_AREYAWINNINGSON], 0);
}