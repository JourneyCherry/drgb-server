#include "MyBattle.hpp"

bool GameInfo::isAllIn()
{
	auto allPlayer = session.GetAll();
	for(auto player : allPlayer)
	{
		auto weak = std::get<2>(player);
		if(weak.expired())
			return false;
		auto shared = weak.lock();
		if(!shared->is_open())
			return false;
	}
	return true;
}

Expected<std::weak_ptr<MyClientSocket>> GameInfo::Enter(Account_ID_t id, std::shared_ptr<MyClientSocket> socket)
{
	std::unique_lock lk(mtx);
	auto origin = session.FindLKey(id)->second;
	if(!session.InsertLKeyValue(id, socket))
		return {};
	timer->cancel();
	return origin;
}

void GameInfo::Exit(Account_ID_t id, std::shared_ptr<MyClientSocket> socket)
{
	std::unique_lock lk(mtx);
	auto sess = session.FindLKey(id);
	if(sess && !sess->second.expired())
	{
		auto ptr = sess->second.lock();
		if(ptr == socket)
		{
			session.InsertLKeyValue(id, std::weak_ptr<MyClientSocket>());
			timer->cancel();
		}
	}
}

MyBattle::MyBattle() : 
	machine_id(ConfigParser::GetInt("Machine_ID", 1)),
	self_keyword("battle" + std::to_string(machine_id)),
	keyword_match("match"),
	BattleService(ConfigParser::GetString("Match_Addr", "localhost"), ConfigParser::GetInt("Service_Port", 52431), machine_id, this),
	gamepool(ConfigParser::GetInt("GameThread", 2), this),
	Round_Time(ConfigParser::GetInt("Round_Time", 2000)),
	Dis_Time(ConfigParser::GetInt("Disconnect_Time", 15000)),
	StartAnim_Time(std::chrono::milliseconds(ConfigParser::GetInt("StartAnimation_Time", 2000)) + Round_Time),
	MyServer(ConfigParser::GetInt("Client_Port", 54321))
{
	BattleService.SetCallback(std::bind(&MyBattle::MatchTransfer, this, std::placeholders::_1, std::placeholders::_2));
}

MyBattle::~MyBattle()
{
}

void MyBattle::Open()
{
	Logger::log("Battle Server Start as " + self_keyword, Logger::LogType::info);
}

void MyBattle::Close()
{
	gamepool.Stop();
	BattleService.Close();
	Logger::log("Battle Server Stop", Logger::LogType::info);
}

void MyBattle::AcceptProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec)
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : " + ec.message_code(), Logger::LogType::auth);
		client->Close();
		return;
	}

	client->StartRecv(std::bind(&MyBattle::AuthenticateProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	client->SetTimeout(TIME_AUTHENTICATE, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
}

void MyBattle::AuthenticateProcess(std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
{
	client->CancelTimeout();
	if(!ec)
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : " + ec.message_code(), Logger::LogType::auth);
		client->Close();
		return;
	}

	Hash_t cookie = packet.pop<Hash_t>();

	//Session Server에서 Session 찾기
	auto info = redis.GetInfoFromCookie(cookie);
	if(!info)	//Session이 없는 경우
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : No Session Found", Logger::LogType::auth);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		client->Close();
		return;
	}
	Account_ID_t account_id = info->first;
	if(info->second != self_keyword)	//Session은 있지만 대상이 Battle 서버가 아닌 경우
	{
		Logger::log("Account " + std::to_string(account_id) + " Failed to Authenticate : Session Mismatched", Logger::LogType::auth);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		client->Close();
		return;
	}

	//Local Server에서 Session찾기
	auto session = sessions.FindRKey(cookie);
	std::shared_ptr<GameInfo> gameinfo;
	int side;
	if(session)
	{
		gameinfo = session->second;
		side = gameinfo->session.FindLKey(account_id)->first;
		auto origin = gameinfo->Enter(account_id, client);
		if(origin)
		{
			if(!origin->expired())
			{
				auto origin_ptr = origin->lock();
				Logger::log("Account " + std::to_string(account_id) + " has dup access : " + origin_ptr->ToString() + " -> " + client->ToString(), Logger::LogType::auth);
				origin_ptr->Send(ByteQueue::Create<byte>(ERR_DUPLICATED_ACCESS));
				origin_ptr->Close();
			}
		}
		else	//올바르지 않은 세션이 올라온 경우(ID가 MatchTransfer로 왔지만 등록이 다른 ID로 된 경우)
		{
			Logger::log("Account " + std::to_string(account_id) + " Failed to Authenticate : Wrong Game Found", Logger::LogType::auth);
			this->sessions.EraseLKey(account_id);
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
			redis.SetInfo(account_id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.
			client->Close();
			return;
		}
	}
	else	//세션이 match로부터 오지 않는 경우
	{
		Logger::log("Account " + std::to_string(account_id) + " Failed to Authenticate : No Matching Game", Logger::LogType::auth);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		redis.SetInfo(account_id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.
		client->Close();
		return;
	}


	//사용자에게 정보 전달.
	{
		ByteQueue info = ByteQueue::Create<byte>(GAME_PLAYER_INFO_NAME);
		info += gameinfo->PlayerInfos[side] + gameinfo->PlayerInfos[MyGame::MAX_PLAYER - (side + 1)];
		client->Send(info);
	}

	Logger::log("Account " + std::to_string(account_id) + " logged in from " + client->ToString(), Logger::LogType::auth);
	ClientProcess(client, account_id, side, gameinfo->game);
	client->SetTimeout(SESSION_TIMEOUT / 2, std::bind(&MyBattle::SessionProcess, this, std::placeholders::_1, account_id, cookie));
}

void MyBattle::ClientProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, int side, std::shared_ptr<MyGame> GameSession)
{
	target_client->StartRecv([this, account_id, side, GameSession](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		if(!recv_ec)
		{
			auto session = sessions.FindLKey(account_id);
			if(session)
				session->second->Exit(account_id, client);
			if(client->isNormalClose(recv_ec))
				Logger::log("Account " + std::to_string(account_id) + " logged out", Logger::LogType::auth);
			else
				Logger::log("Account " + std::to_string(account_id) + " logged out with " + recv_ec.message_code(), Logger::LogType::auth);
			client->Close();
			return;
		}

		byte header = packet.pop<byte>();
		switch(header)
		{
			case ANS_HEARTBEAT:
				break;
			case REQ_GAME_ACTION:
				try
				{
					int action = packet.pop<int>();
					GameSession->Action(side, action);
				}
				catch(const std::exception &e)
				{
					client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
					Logger::log(e.what(), Logger::LogType::error);
				}
				break;
			default:
				client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
				break;
		}
		ClientProcess(client, account_id, side, GameSession);
	});
}

void MyBattle::SessionProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, Hash_t cookie)
{
	target_client->SetTimeout(SESSION_TIMEOUT / 2, [this, account_id, cookie](std::shared_ptr<MyClientSocket> client)
	{
		if(!client->is_open())
			return;

		if(!redis.RefreshInfo(account_id, cookie, self_keyword))
			client->Close();
		else
		{
			client->Send(ByteQueue::Create<byte>(ANS_HEARTBEAT));
			SessionProcess(client, account_id, cookie);
		}
	});
}

void MyBattle::GameProcess(std::shared_ptr<boost::asio::steady_timer> timer, std::shared_ptr<GameInfo> gameinfo)
{
	if(!gamepool.is_open())
		return;

	timer->async_wait([this, timer, gameinfo](const boost::system::error_code &error_code)
	{
		bool prev_allin = gameinfo->Previous_AllIn;
		bool now_allin = gameinfo->isAllIn();
		gameinfo->Previous_AllIn = now_allin;

		if(error_code.failed())	//사용자가 접속/해제 할 경우 발생.
		{
			if(now_allin)	//2명 모두 접속 or Dup-Access
			{
				SendRoundResult(GAME_PLAYER_ALL_CONNECTED, gameinfo);
				timer->expires_after(StartAnim_Time);
			}
			else if(prev_allin)	//1명 이상 나감
			{
				auto players = gameinfo->session.GetAll();
				for(auto player : players)
				{
					auto ptr = std::get<2>(player).lock();
					if(ptr != nullptr)
						ptr->Send(ByteQueue::Create<byte>(GAME_PLAYER_DISCONNEDTED));
				}
				timer->expires_after(Dis_Time);
			}
			//else	//2명 모두 나감
			GameProcess(timer, gameinfo);
			return;
		}

		auto game = gameinfo->game;
		if(now_allin)	//정상 플레이
		{
			bool isGameOver = game->process();
			SendRoundResult(GAME_ROUND_RESULT, gameinfo);

			auto achievement = game->GetAchievement();
			while(achievement)
			{
				auto [side, achieve, count] = *achievement;
				auto player = gameinfo->session.FindRKey(side);
				auto id = player->first;
				auto socket = player->second.lock();
				AchieveProgress(id, achieve, count, socket);
				achievement = game->GetAchievement();
			}

			if(!isGameOver)
			{
				timer->expires_after(Round_Time);
				GameProcess(timer, gameinfo);
				return;
			}
		}

		std::exception_ptr eptr = nullptr;
		std::array<Account_ID_t, MyGame::MAX_PLAYER> ids;
		try
		{
			std::array<std::shared_ptr<MyClientSocket>, MyGame::MAX_PLAYER> sockets;
			for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			{
				auto info = gameinfo->session.FindRKey(i);
				ids[i] = info->first;
				sockets[i] = info->second.lock();
			}

			int winner = game->GetWinner();
			const bool drawed = (winner < 0);
			const bool crashed = !now_allin;

			//Log & Achievement(Draw)
			const std::string logtitlestr = "Game (" + std::to_string(ids[0]) + " vs " + std::to_string(ids[1]) + ")";
			if(drawed && !crashed)	//무승부의 경우.
			{
				for(int i = 0;i<MyGame::MAX_PLAYER;i++)
					AchieveProgress(ids[i], ACHIEVE_AREYAWINNINGSON, 1, sockets[i]);
				Logger::log(logtitlestr + " : Draw", Logger::LogType::info);
			}
			else if(drawed && crashed)	//게임이 취소된 경우.
				Logger::log(logtitlestr + " : Crashed by disconnect", Logger::LogType::info);
			else
				Logger::log(logtitlestr + " : " + std::to_string(ids[winner]) + " Win", Logger::LogType::info);

			//Battle Archive
			try
			{
				auto db = dbpool.GetConnection();
				if(!db)
					throw pqxx::broken_connection();

				if(drawed)
					db->ArchiveBattle(ids[0], ids[1], drawed, crashed);
				else
					db->ArchiveBattle(ids[winner], ids[1 - winner], drawed, crashed);
				db->commit();
			}
			catch(const pqxx::pqxx_exception &e)
			{
				Logger::log("DB Error : " + std::string(e.base().what()), Logger::LogType::error);
				for(int i = 0;i<MyGame::MAX_PLAYER;i++)
					sockets[i]->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
				std::rethrow_exception(std::current_exception());
			}
			catch(const std::exception &e)
			{
				Logger::log("Error : " + std::string(e.what()), Logger::LogType::error);
				for(int i = 0;i<MyGame::MAX_PLAYER;i++)
					sockets[i]->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
				std::rethrow_exception(std::current_exception());
			}
			
			//Win Achievement Count
			if(!drawed)
			{
				AchieveProgress(ids[winner], ACHIEVE_NOIVE, 1, sockets[winner]);
				AchieveProgress(ids[winner], ACHIEVE_CHALLENGER, 1, sockets[winner]);
				AchieveProgress(ids[winner], ACHIEVE_DOMINATOR, 1, sockets[winner]);
				AchieveProgress(ids[winner], ACHIEVE_SLAYER, 1, sockets[winner]);
				AchieveProgress(ids[winner], ACHIEVE_CONQUERER, 1, sockets[winner]);
			}

			//Session Erase & Send Game Result
			for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			{
				auto erase_result = sessions.EraseLKey(ids[i]);
				if(!erase_result)	//session이 없으면 이미 빠진 것이므로 무시.
					continue;
				auto [id, cookie, _g] = *erase_result;
				if(sockets[i] == nullptr || !sockets[i]->is_open())	//연결이 끊어졌으면 게임 중간에 나간 것이므로 무시.
				{
					redis.ClearInfo(id, cookie, self_keyword);	//단, 이미 나갔기 때문에 세션은 지운다.
					continue;
				}
				redis.SetInfo(id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.

				byte gameresult;
				if(crashed && drawed)	gameresult = GAME_CRASHED;
				else if(drawed)			gameresult = GAME_FINISHED_DRAW;
				else if(winner == i)	gameresult = GAME_FINISHED_WIN;
				else					gameresult = GAME_FINISHED_LOOSE;
				ByteQueue packet = ByteQueue::Create<byte>(gameresult);
				sockets[i]->Send(packet);
				sockets[i]->Close();
			}
		}
		catch(StackTraceExcept e)
		{
			e.stack(__STACKINFO__);
			eptr = std::make_exception_ptr(e);
		}
		catch(const std::exception &e)
		{
			eptr = std::make_exception_ptr(StackTraceExcept(e.what(), __STACKINFO__));
		}

		gamepool.ReleaseTimer(timer);
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			sessions.EraseLKey(ids[i]);	//exception 발생할 시를 대비한 쿠키 지우기. 이미 예외가 발생했으므로, 후처리는 못한다고 봐야 한다.

		if(eptr)
			std::rethrow_exception(eptr);
	});
}

void MyBattle::MatchTransfer(const Account_ID_t& lpid, const Account_ID_t& rpid)
{
	if(lpid <= 0 || rpid <= 0)
	{
		BattleService.SetUsage(sessions.Size());
		return;
	}

	std::array<Account_ID_t, MyGame::MAX_PLAYER> ids({lpid, rpid});
	auto timer = gamepool.GetTimer();
	if(timer)
	{
		auto lpresult = redis.GetInfoFromID(lpid);
		auto rpresult = redis.GetInfoFromID(rpid);
		if(lpresult && rpresult)
		{
			std::array<Hash_t, MyGame::MAX_PLAYER> cookies({lpresult->first, rpresult->first});
			std::shared_ptr<MyGame> new_game = std::make_shared<MyGame>();

			std::shared_ptr<GameInfo> gameinfo = std::make_shared<GameInfo>(new_game, timer);
			for(int i = 0;i<MyGame::MAX_PLAYER;i++)
				gameinfo->session.Insert(ids[i], i, std::weak_ptr<MyClientSocket>());

			try
			{
				auto db = dbpool.GetConnection();
				if(!db)
					throw pqxx::broken_connection();

				for(int i = 0;i<MyGame::MAX_PLAYER;i++)
				{
					auto [name, win, draw, loose] = db->GetInfo(ids[i]);
					gameinfo->PlayerInfos[i].push<Achievement_ID_t>(name);	//nickname
					gameinfo->PlayerInfos[i].push<int>(win);	//win
					gameinfo->PlayerInfos[i].push<int>(draw);	//draw
					gameinfo->PlayerInfos[i].push<int>(loose);	//loose
				}
			}
			catch(const pqxx::pqxx_exception & e)
			{
				Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::error);
				gamepool.ReleaseTimer(timer);
				BattleService.SetUsage(sessions.Size());
				return;
			}
			catch(const std::exception &e)
			{
				Logger::log("DB Failed : " + std::string(e.what()), Logger::LogType::error);
				gamepool.ReleaseTimer(timer);
				BattleService.SetUsage(sessions.Size());
				return;
			}

			for(int i = 0;i<MyGame::MAX_PLAYER;i++)
				sessions.Insert(ids[i], cookies[i], gameinfo);

			Logger::log("Game (" + std::to_string(lpid) + " vs " + std::to_string(rpid) + ") : Transfered", Logger::LogType::info);
			timer->expires_after(Dis_Time);	//최초 Timer 등록
			GameProcess(timer, gameinfo);	//최초 Timer 등록
		}
	}
	BattleService.SetUsage(sessions.Size());
}

void MyBattle::SendRoundResult(const byte& msg, std::shared_ptr<GameInfo> gameinfo)
{
	std::array<ByteQueue, MyGame::MAX_PLAYER> results;
	ByteQueue header = ByteQueue::Create<byte>(msg);
	int round = gameinfo->game->GetRound();
	header.push<int>(round);

	if(round >= 0)
	{
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
		{
			auto [action, health, energy] = gameinfo->game->GetPlayerInfo(i);
			results[i].push<byte>(action);
			results[i].push<int>(health);
			results[i].push<int>(energy);
		}
	}

	auto players = gameinfo->session.GetAll();
	for(auto player : players)
	{
		auto side = std::get<1>(player);
		auto ptr = std::get<2>(player).lock();
		if(ptr != nullptr)
			ptr->Send(header + results[side] + results[MyGame::MAX_PLAYER - (side + 1)]);
	}
}

void MyBattle::AchieveProgress(Account_ID_t id, Achievement_ID_t achieve, int count, std::shared_ptr<MyClientSocket> socket)
{
	try
	{
		auto db = dbpool.GetConnection();
		if(!db)
			throw pqxx::broken_connection();
		bool isAchieved = false;
		switch(achieve)
		{
			//(행위의)최대 횟수만 카운팅 하는 업적
			case ACHIEVE_MEDITATE_ADDICTION:
			case ACHIEVE_GUARD_ADDICTION:
			case ACHIEVE_EVADE_ADDICTION:
			case ACHIEVE_PUNCH_ADDICTION:
				isAchieved = db->AchieveProgress(id, achieve, count);
				break;
			//매 회마다 횟수가 1씩 증가하는 업적
			case ACHIEVE_FIRE_ADDICTION:
			case ACHIEVE_NEWBIE:
			case ACHIEVE_NOIVE:
			case ACHIEVE_CHALLENGER:
			case ACHIEVE_DOMINATOR:
			case ACHIEVE_SLAYER:
			case ACHIEVE_CONQUERER:
			case ACHIEVE_KNEELINMYSIGHT:
			case ACHIEVE_DOPPELGANGER:
			case ACHIEVE_AREYAWINNINGSON:
				isAchieved = db->Achieve(id, achieve);
				break;
		}
		if(isAchieved)
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

size_t MyBattle::GetUsage()
{
	return gamepool.Size();
}

bool MyBattle::CheckAccount(Account_ID_t id)
{
	return sessions.FindLKey(id).isSuccessed();
}

std::map<std::string, size_t> MyBattle::GetConnectUsage()
{
	auto connected = MyServer::GetConnectUsage();
	connected.insert({"match", BattleService.is_open()?1:0});
	return connected;
}