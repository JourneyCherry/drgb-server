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
const std::chrono::seconds MyGame::StartAnim_Time = std::chrono::seconds(2) + MyGame::Round_Time;	//Start Animation이 그려진 뒤, 자동으로 한 라운드를 진행하므로 사용자 입력을 기다리는 Round_Time이 추가되어야 한다.

MyGame::MyGame(Account_ID_t lpid, Account_ID_t rpid, std::shared_ptr<boost::asio::steady_timer> t, MyPostgresPool *pool)
 : timer(t), state_level(0), now_round(-1), dbpool(pool)
{
	ulock lk(mtx);
	players[0].id = lpid;
	players[1].id = rpid;
	logtitlestr = "Game (" + std::to_string(players[0].id) + " vs " + std::to_string(players[1].id) + ")";
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		players[i].socket = nullptr;
		players[i].action = MEDITATE;
		players[i].health = MAX_HEALTH;
		players[i].energy = 0;
		players[i].target = (MAX_PLAYER - 1) - i;	//한 게임의 플레이어 수가 2명 초과일 경우, 플레이어가 직접 타겟을 정할 수 있도록 변경 필요.
		players[i].result = 0;
		players[i].prev_action = -1;
		players[i].consecutive = 0;
		try
		{
			auto db = dbpool->GetConnection();
			if(!db)
				throw pqxx::broken_connection();
			auto [name, win, draw, loose] = db->GetInfo(players[i].id);
			players[i].info.push<Achievement_ID_t>(name);	//nickname
			players[i].info.push<int>(win);	//win
			players[i].info.push<int>(draw);	//draw
			players[i].info.push<int>(loose);	//loose
		}
		catch(const pqxx::pqxx_exception & e)
		{
			Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::error);
		}
		catch(const std::exception &e)
		{
			Logger::log("DB Failed : " + std::string(e.what()), Logger::LogType::error);
		}
	}
}

int MyGame::Connect(Account_ID_t id, std::shared_ptr<MyClientSocket> socket)
{
	ulock lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].id == id)
		{
			if(players[i].result != 0)	//초기값이 아닌 경우 이미 게임이 종료된 것.
				return -1;
			auto existing = players[i].socket;
			if(existing != nullptr)
			{
				lk.unlock();
				Disconnect(i, existing);
				Logger::log("Account " + std::to_string(id) + " has dup access : " + existing->ToString() + " -> " + socket->ToString(), Logger::LogType::auth);
				existing->Send(ByteQueue::Create<byte>(ERR_DUPLICATED_ACCESS));
				existing->Close();
				lk.lock();
			}
			players[i].socket = socket;
			timer->cancel();
			return i;
		}
	}

	return -1;
}

void MyGame::Disconnect(int side, std::shared_ptr<MyClientSocket> socket)
{
	ulock lk(mtx);
	if(players[side].socket == socket)
	{
		players[side].socket = nullptr;
		timer->cancel();
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

bool MyGame::Work(const bool& interrupted)
{
	switch(state_level)
	{
		case 0:	//최초 호출
			timer->expires_after(Dis_Time);
			state_level++;
			return true;
		case 1:		//Player 최초 접속 기다리기.
			if(isAllIn())
			{
				state_level++;
				SendAll(ByteQueue::Create<byte>(GAME_PLAYER_ALL_CONNECTED) + ByteQueue::Create<int>(now_round++));
				timer->expires_after(StartAnim_Time);
				return true;
			}
			if(!interrupted)
			{
				ulock lk(mtx);
				for(int i = 0;i<MAX_PLAYER;i++)
					players[i].result = GAME_CRASHED;
				lk.unlock();
				
				ProcessResult(true, true);
				return false;
			}
			return true;
		case 2:		//Game Play
			if(interrupted)
			{
				if(!isAllIn())
				{
					SendAll(ByteQueue::Create<byte>(GAME_PLAYER_DISCONNEDTED));
					timer->expires_after(Dis_Time);
					state_level++;
				}
			}
			else
			{
				bool isGameOver = process();
				//라운드 결과 전달하기.
				ByteQueue result = ByteQueue::Create<byte>(GAME_ROUND_RESULT);
				result.push<int>(now_round);

				ulock lk(mtx);
				for(int i = 0;i<MAX_PLAYER;i++)
				{
					if(players[i].socket != nullptr)	//process() 진행 중, player가 나가는 경우가 있으면 에러가 발생함.
						players[i].socket->Send(result + GetPlayerByte(std::ref(players[i])) + GetPlayerByte(std::ref(players[players[i].target])));
				}
				for(int i = 0;i<MAX_PLAYER;i++)
					players[i].action = MEDITATE;
				lk.unlock();

				if(++now_round >= MAX_ROUND || isGameOver)
				{
					ProcessResult(!isGameOver, false);
					return false;
				}

				if(!isAllIn())
				{
					SendAll(ByteQueue::Create<byte>(GAME_PLAYER_DISCONNEDTED));
					timer->expires_after(Dis_Time);
					state_level++;
				}
				else
					timer->expires_after(Round_Time);
			}
			return true;
		case 3:		//Disconnected
			if(isAllIn())
			{
				now_round--;		//해당 라운드를 무효 한 뒤,

				ByteQueue result = ByteQueue::Create<byte>(GAME_PLAYER_ALL_CONNECTED);	//사용자들에게 경기재개 패킷과
				result.push<int>(now_round);	//이전 라운드의 결과를 보내준다.

				if(now_round >= 0)	//최초 라운드는 결과를 보내지 않는다.(아직 진행된 내용이 없으므로)
				{
					ulock lk(mtx);
					for(int i = 0;i<MAX_PLAYER;i++)
					{
						if(players[i].socket != nullptr)
							players[i].socket->Send(result + GetPlayerByte(std::ref(players[i])) + GetPlayerByte(std::ref(players[players[i].target])));
					}
					for(int i = 0;i<MAX_PLAYER;i++)
						players[i].action = MEDITATE;
				}
				else
					SendAll(result);

				timer->expires_after(StartAnim_Time);
				state_level--;
				return true;
			}
			if(!interrupted)	//Disconnection Time이 지날떄까지 들어오지 못하면 해당 게임은 종료된다.
			{
				bool drawed = (now_round <= DODGE_ROUND || GetWinner() < 0);	//일정 라운드 이내거나 둘다 연결이 종료되면 무효. 그 외엔 나간 쪽이 loose
				ProcessResult(drawed, true);
				return false;
			}
			return true;
		default:
			return false;
	}
}


void MyGame::ProcessResult(bool drawed, bool crashed)
{
	//DB에 경기결과 기록 및 사용자들에게 경기결과 전달 + 연결 종료.
	int winner = GetWinner();
	if(winner < 0)	//경기 결과는 승패가 나뉘었지만 최종판결 전에 플레이어가 모두 나가는 경우, 무승부처리된다.
		drawed = true;
	if(drawed && !crashed)	//무승부의 경우.
	{
		for(int i = 0;i<MAX_PLAYER;i++)
			AchieveCount(players[i].id, ACHIEVE_AREYAWINNINGSON, players[i].socket);
		Logger::log(logtitlestr + " : Draw", Logger::LogType::info);
	}
	else if(drawed && crashed)	//게임이 취소된 경우.
		Logger::log(logtitlestr + " : Crashed by disconnect", Logger::LogType::info);
	else
		Logger::log(logtitlestr + " : " + std::to_string(players[winner].id) + " Win", Logger::LogType::info);

	try
	{
		auto db = dbpool->GetConnection();
		if(!db)
			throw pqxx::broken_connection();
		Account_ID_t winner_id = players[0].id;
		Account_ID_t looser_id = players[1].id;
		if(winner == 1)
			std::swap(winner_id, looser_id);

		db->ArchiveBattle(winner_id, looser_id, drawed, crashed);
		db->commit();
	}
	catch(const pqxx::pqxx_exception &e)
	{
		Logger::log("DB Error : " + std::string(e.base().what()), Logger::LogType::error);
		SendAll(ByteQueue::Create<byte>(ERR_DB_FAILED));
		std::rethrow_exception(std::current_exception());
	}
	catch(const std::exception &e)
	{
		Logger::log("Error : " + std::string(e.what()), Logger::LogType::error);
		SendAll(ByteQueue::Create<byte>(ERR_DB_FAILED));
		std::rethrow_exception(std::current_exception());
	}
	
	if(winner >= 0)
	{
		AchieveCount(players[winner].id, ACHIEVE_NOIVE, players[winner].socket);
		AchieveCount(players[winner].id, ACHIEVE_CHALLENGER, players[winner].socket);
		AchieveCount(players[winner].id, ACHIEVE_DOMINATOR, players[winner].socket);
		AchieveCount(players[winner].id, ACHIEVE_SLAYER, players[winner].socket);
		AchieveCount(players[winner].id, ACHIEVE_CONQUERER, players[winner].socket);
	}

	//결과에 따라 승/패가 나뉘기 때문에 SendAll()로 보낼 수 없어, 따로 보냄.
	ulock lock(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(crashed && drawed)	players[i].result = GAME_CRASHED;
		else if(drawed)			players[i].result = GAME_FINISHED_DRAW;
		else if(winner == i)	players[i].result = GAME_FINISHED_WIN;
		else					players[i].result = GAME_FINISHED_LOOSE;
		//결과를 보낼 때, match server에 세션을 넘겨주면서 match server 정보도 넘겨야 하므로, 이것은 MyBattle::pool_manage에서 처리한다.
		//if(players[i].socket->Send(ByteQueue::Create<byte>(result)))
		//	players[i].socket->Close();	//여기서 연결을 종료하면 MyBattle::ClientProcess()에서 연결끊김을 감지하여 알아서 스레드를 종료하게 된다.
	}
}

bool MyGame::process()
{
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
					AchieveCount(players[i].id, ACHIEVE_MEDITATE_ADDICTION, players[i].socket);
					players[i].energy = MAX_ENERGY;
				}
				break;
			case PUNCH:
				if(players[target].action & 0x01)	//meditate
					players[target].health -= 1;
				if(players[target].action == players[i].action)
					AchieveCount(players[i].id, ACHIEVE_DOPPELGANGER, players[i].socket);
				AchieveProgress(players[i].id, ACHIEVE_PUNCH_ADDICTION, players[i].consecutive, players[i].socket);	//동일 행동 2회 이상. 단, 최대 진행도를 기록하기 위해, 별도로 consecutive가 2 이상인지 체크하지 않는다.
				break;
			case FIRE:
				if(players[target].action & 0x0b)	//meditate, guard, punch
				{
					players[target].health -= 1;
					if(players[target].health <= 0 && players[target].action == PUNCH)
						AchieveCount(players[i].id, ACHIEVE_KNEELINMYSIGHT, players[i].socket);
				}
				if(players[target].action == players[i].action)
					AchieveCount(players[i].id, ACHIEVE_DOPPELGANGER, players[i].socket);
				AchieveCount(players[i].id, ACHIEVE_FIRE_ADDICTION, players[i].socket);
				break;
			case GUARD:
			case EVADE:
				//방어계열은 별도의 동작을 수행하지 않는다.
				AchieveProgress(players[i].id, players[i].action == GUARD?ACHIEVE_GUARD_ADDICTION:ACHIEVE_EVADE_ADDICTION, players[i].consecutive, players[i].socket);	//동일 행동 2회 이상. 단, 최대 진행도를 기록하기 위해, 별도로 consecutive가 2 이상인지 체크하지 않는다.
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

bool MyGame::isAllIn()
{
	std::unique_lock<std::mutex> lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(!players[i].socket)
			return false;
	}
	return true;
}

void MyGame::SendAll(ByteQueue byte)
{
	std::unique_lock lk(mtx);
	for(int i = 0;i<MAX_PLAYER;i++)
	{
		if(players[i].socket != nullptr)
			players[i].socket->Send(byte);
	}
}

ByteQueue MyGame::GetPlayerByte(const struct player_info &player)
{
	ByteQueue result = ByteQueue::Create<byte>(player.action);
	result.push<int>(player.health);
	result.push<int>(player.energy);

	return result;
}

void MyGame::AchieveCount(Account_ID_t id, Achievement_ID_t achieve, std::shared_ptr<MyClientSocket> socket)
{
	try
	{
		auto db = dbpool->GetConnection();
		if(!db)
			throw pqxx::broken_connection();
		if(db->Achieve(id, achieve))
		{
			ByteQueue packet = ByteQueue::Create<byte>(GAME_PLAYER_ACHIEVE);
			packet.push<Achievement_ID_t>(achieve);
			if(socket != nullptr && socket->is_open())
				socket->Send(packet);
		}
	}
	catch(const pqxx::pqxx_exception & e)
	{
		Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::error);
	}
	catch(const std::exception &e)
	{
		Logger::log("DB Failed : " + std::string(e.what()), Logger::LogType::error);
	}
}

void MyGame::AchieveProgress(Account_ID_t id, Achievement_ID_t achieve, int count, std::shared_ptr<MyClientSocket> socket)
{
	try
	{
		auto db = dbpool->GetConnection();
		if(!db)
			throw pqxx::broken_connection();
		if(db->AchieveProgress(id, achieve, count))
		{
			ByteQueue packet = ByteQueue::Create<byte>(GAME_PLAYER_ACHIEVE);
			packet.push<Achievement_ID_t>(achieve);
			if(socket != nullptr && socket->is_open())
				socket->Send(packet);
		}
	}
	catch(const pqxx::pqxx_exception & e)
	{
		Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::error);
	}
	catch(const std::exception &e)
	{
		Logger::log("DB Failed : " + std::string(e.what()), Logger::LogType::error);
	}
}