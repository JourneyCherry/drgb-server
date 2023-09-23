#include "MyGame.hpp"

const std::map<int, int> MyGame::required_energy = 
{
	{MEDITATE, 0},
	{GUARD, 0},
	{EVADE, 1},
	{PUNCH, 1},
	{FIRE, 2}
};

MyGame::MyGame() : now_round(-1)
{
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		players[i].action = MEDITATE;
		players[i].health = MAX_HEALTH;
		players[i].energy = 0;
		players[i].target = MAX_PLAYER - (i + 1);
		players[i].prev_action = -1;
		players[i].consecutive = 0;
	}
}

void MyGame::Action(int side, int action)
{
	if(!CheckAction(action))	//유효하지 않은 행동.
		return;
	ulock lk(mtx);
	if(required_energy.at(action) <= players[side].energy)
		players[side].action = action;
}

int MyGame::GetWinner() const
{
	int winner = -1;	//아무도 생존 못할 경우, 무승부 처리.
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].health > 0)
		{
			if(winner >= 0)
				return -1;	//1명 초과 생존의 경우, 무승부 처리.(게임이 아직 완료되지 않음)
			winner = i;
		}
	}
	return winner;
}

bool MyGame::process()
{
	if(now_round >= MAX_ROUND)
		return true;
	if(GetWinner() >= 0)
		return true;

	now_round++;
	std::unique_lock lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		int prev = players[i].prev_action;
		int action = players[i].action;
		int target = players[i].target;
		players[i].prev_action = action;

		players[i].consecutive++;
		if(prev != action)
			players[i].consecutive = 1;

		auto iter = required_energy.find(action);
		if(iter == required_energy.end())
			continue;

		int use_energy = iter->second;
		if(players[i].energy < use_energy)
			continue;

		switch(players[i].action)
		{
			case MEDITATE:
				players[i].energy += 1;
				if(players[i].energy > MAX_ENERGY)
				{
					AchieveProgress(i, ACHIEVE_MEDITATE_ADDICTION, 1);
					players[i].energy = MAX_ENERGY;
				}
				break;
			case PUNCH:
				if(players[target].action & 0x01)	//meditate
					players[target].health -= 1;
				if(players[target].action == players[i].action)
					AchieveProgress(i, ACHIEVE_DOPPELGANGER, 1);
				AchieveProgress(i, ACHIEVE_PUNCH_ADDICTION, players[i].consecutive);	//동일 행동 2회 이상. 단, 최대 진행도를 기록하기 위해, 별도로 consecutive가 2 이상인지 체크하지 않는다.
				break;
			case FIRE:
				if(players[target].action & 0x0b)	//meditate, guard, punch
				{
					players[target].health -= 1;
					if(players[target].health <= 0 && players[target].action == PUNCH)
						AchieveProgress(i, ACHIEVE_KNEELINMYSIGHT, 1);
				}
				if(players[target].action == players[i].action)
					AchieveProgress(i, ACHIEVE_DOPPELGANGER, 1);
				AchieveProgress(i, ACHIEVE_FIRE_ADDICTION, 1);
				break;
			case GUARD:
			case EVADE:
				//방어계열은 별도의 동작을 수행하지 않는다.
				AchieveProgress(i, players[i].action == GUARD?ACHIEVE_GUARD_ADDICTION:ACHIEVE_EVADE_ADDICTION, players[i].consecutive);	//동일 행동 2회 이상. 단, 최대 진행도를 기록하기 위해, 별도로 consecutive가 2 이상인지 체크하지 않는다.
				break;
			default:
				continue;
		}
		players[i].energy -= use_energy;
	}

	//Initialize Input
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		players[i].action = MEDITATE;
	}

	return (GetWinner() >= 0 || now_round >= MAX_ROUND);
}

bool MyGame::CheckAction(int action)
{
	if(action & ~0x1F)	return false;	//범위 초과 영역에 비트가 있으면 false

	if(MYPOPCNT(static_cast<unsigned int>(action)) != 1) return false;	//1 비트가 1개가 아니면 여러 액션을 지칭하거나 없을 수 있으므로 false.

	return true;
}

std::tuple<byte, int, int> MyGame::GetPlayerInfo(const int& side) const
{
	if(side < 0 || side > 1)
		return {0, -1, -1};
	auto player = players[side];
	return {player.prev_action, player.health, player.energy};
}

int MyGame::GetRound() const
{
	return now_round;
}

void MyGame::AchieveProgress(int side, Achievement_ID_t achieve, int count)
{
	Achievements.push(std::make_tuple(side, achieve, count));
}

Expected<std::tuple<int, Achievement_ID_t, int>> MyGame::GetAchievement()
{
	if(Achievements.empty())
		return {};
	auto result = Achievements.front();
	Achievements.pop();
	return result;
}