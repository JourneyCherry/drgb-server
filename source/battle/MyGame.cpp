#include "MyGame.hpp"

const std::map<int, int> MyGame::required_energy = 
{
	{MEDITATE, 0},
	{GUARD, 0},
	{EVADE, 1},
	{PUNCH, 1},
	{FIRE, 2}
};
const std::chrono::seconds MyGame::Round_Time = std::chrono::seconds(2);
const std::chrono::seconds MyGame::Dis_Time = std::chrono::seconds(15);

MyGame::MyGame(Account_ID_t lpid, Account_ID_t rpid)
{
	ulock lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		players[i].id = 0;
		players[i].socket = nullptr;
		players[i].action = MEDITATE;
		players[i].health = MAX_HEALTH;
		players[i].energy = 0;
		players[i].target = (MAX_PLAYER - 1) - i;	//한 게임의 플레이어 수가 2명 초과일 경우, 플레이어가 직접 타겟을 정할 수 있도록 변경 필요.
	}
	players[0].id = lpid;
	players[1].id = rpid;
}

int MyGame::Connect(Account_ID_t id, std::shared_ptr<MyClientSocket> socket)
{
	ulock lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].id == id)
		{
			if(socket != nullptr)
			{
				socket->Send(MyBytes::Create<byte>(ERR_DUPLICATED_ACCESS));
				socket->Close();
			}
			players[i].socket = socket;
			cv.notify_all();
			return i;
		}
	}

	return -1;
}

void MyGame::Disconnect(int side, std::shared_ptr<MyClientSocket> socket)
{
	ulock lk(mtx);
	if(players[side].socket == socket)	//Duplicated Access로 인해 변경됬을 경우엔 적용하지 않는다.
	{
		players[side].socket = nullptr;
		cv.notify_all();
	}
}

void MyGame::Action(int side, int action)
{
	if(!CheckAction(action))	//유효하지 않은 행동.
		return;
	ulock lk(mtx);
	players[side].action = action;
}

int MyGame::GetWinner()
{
	ulock lk(mtx);
	int winner = -1;	//아무도 생존 못할 경우, 무승부 처리.
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].socket == nullptr)	//플레이어가 연결되어있지 않으면 사망처리한다.
			continue;
		if(players[i].health > 0)
		{
			if(winner >= 0)
				return -1;	//1명 초과 생존의 경우, 무승부 처리.(한쪽의 닷지 or 라운드 초과)
			winner = i;
		}
	}
	return winner;
}

void MyGame::Work()
{
	std::function<bool()> isAllIn = [&](){
		for(int i = 0;i<MAX_PLAYER;i++)
		{
			if(!players[i].socket)
				return false;
		}
		return true;
	};
	//Player 최초 접속 기다리기.(대기시간 : 무한대)
	{
		ulock lk(mtx);
		cv.wait(lk, isAllIn);
		SendAll(MyBytes::Create<byte>(GAME_PLAYER_ALL_CONNECTED));
	}

	//실제 플레이 내용
	bool isGameOver = false;
	for(int now_round = 0;now_round < MAX_ROUND && !isGameOver;now_round++)
	{
		ulock lk(mtx);
		bool isTimeOver = cv.wait_for(lk, Round_Time, isAllIn);
		if(isTimeOver)
		{
			isGameOver = process();
			//라운드 결과 전달하기.
			MyBytes result = MyBytes::Create<byte>(GAME_ROUND_RESULT);
			result.push<int>(now_round);

			for(int i = 0;i<MAX_PLAYER;i++)
			{
				players[i].socket->Send(result + GetPlayerByte(std::ref(players[i])) + GetPlayerByte(std::ref(players[players[i].target])));
				players[i].action = MEDITATE;
			}
			lk.unlock();
		}
		else
		{
			SendAll(MyBytes::Create<byte>(GAME_PLAYER_DISCONNEDTED));
			bool isAllConnected = cv.wait_for(lk, Dis_Time, isAllIn);
			lk.unlock();
			if(!isAllConnected)	//Disconnection Time이 지날떄까지 들어오지 못하면 해당 게임은 종료된다.
			{
				isGameOver = true;
				if(now_round <= DODGE_ROUND || GetWinner() < 0)	//일정 라운드 이내거나 둘다 연결이 종료되면 무효. 그 외엔 나간 쪽이 loose
				{
					SendAll(MyBytes::Create<byte>(GAME_CRASHED), true);
					return;
				}
			}
			else
			{
				SendAll(MyBytes::Create<byte>(GAME_PLAYER_ALL_CONNECTED));	//사용자들에게 경기재개 패킷을 보낸 뒤
				now_round--;		//해당 라운드를 무효 한다.
			}
		}
	}

	//DB에 경기결과 기록 및 사용자들에게 경기결과 전달 + 연결 종료.
	int winner = GetWinner();
	try
	{
		MyPostgres db;
		static std::string query = "SELECT (win_count, draw_count, loose_count) FROM userlist WHERE id=";
		for(int i = 0;i<MAX_PLAYER;i++)
		{
			std::string idstr = std::to_string(players[i].id);
			auto result = db.exec1(query + idstr);
			int win = result[0].as<unsigned int>();
			int draw = result[1].as<unsigned int>();
			int loose = result[2].as<unsigned int>();
			std::string targetstr;
			if(winner < 0)
				targetstr = "draw_count=" + std::to_string(draw+1);
			else if(winner == i)
				targetstr = "win_count=" + std::to_string(win+1);
			else
				targetstr = "loose_count=" + std::to_string(loose+1);

			db.exec("UPDATE " + targetstr + " FROM userlist WHERE id=" + idstr);
		}
		db.commit();
	}
	catch(const std::exception &e)
	{
		SendAll(MyBytes::Create<byte>(ERR_DB_FAILED), true);
		throw;
	}

	//결과에 따라 승/패가 나뉘기 때문에 SendAll()로 보낼 수 없어, 따로 보냄.
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		players[i].socket->Send(winner<0?GAME_FINISHED_DRAW:(winner==i?GAME_FINISHED_WIN:GAME_FINISHED_LOOSE));
		players[i].socket->Close();	//여기서 연결을 종료하면 MyBattle::ClientProcess()에서 연결끊김을 감지하여 알아서 스레드를 종료하게 된다.
	}
}

bool MyGame::process()
{
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		int action = players[i].action;
		int target = players[i].target;
		auto iter = required_energy.find(action);
		if(iter == required_energy.end())
			continue;

		int use_energy = iter->second;
		if(players[i].energy < use_energy)
			continue;

		switch(players[i].action)
		{
			case MEDITATE:
				players[i].energy = std::min(MAX_ENERGY, players[i].energy + 1);
				break;
			case PUNCH:
				if(players[target].action & 0x01)	//meditate
					players[target].health -= 1;
				break;
			case FIRE:
				if(players[target].action & 0x07)	//meditate, guard, punch
					players[target].health -= 1;
				break;
			case GUARD:
			case EVADE:
				//방어계열은 별도의 동작을 수행하지 않는다.
				break;
			default:
				continue;
		}
		players[i].energy -= use_energy;
	}

	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].health <= 0)
			return true;
	}

	return false;
}

bool MyGame::CheckAction(int action)
{
	if(action & ~0x1F)	return false;	//범위 초과 영역에 비트가 있으면 false

	if(MYPOPCNT(static_cast<unsigned int>(action)) != 1) return false;	//1 비트가 1개가 아니면 여러 액션을 지칭하거나 없을 수 있으므로 false.

	return true;
}

void MyGame::SendAll(MyBytes byte, bool connectclose)
{
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].socket != nullptr)
		{
			players[i].socket->Send(byte);
			if(connectclose)
				players[i].socket->Close();
		}
	}
}

MyBytes MyGame::GetPlayerByte(const struct player_info &player)
{
	MyBytes result = MyBytes::Create<byte>(player.action);
	result.push<int>(player.health);
	result.push<int>(player.energy);

	return result;
}