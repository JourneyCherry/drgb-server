#include "MyBattle.hpp"

MyBattle::MyBattle() : 
	machine_id(ConfigParser::GetInt("Machine_ID", 1)),
	self_keyword("battle" + std::to_string(machine_id)),
	keyword_match("match"),
	BattleService(ConfigParser::GetString("Match_Addr"), ConfigParser::GetInt("Service_Port", 52431), machine_id, this),
	gamepool(1, this),
	MyServer(ConfigParser::GetInt("Battle_ClientPort", 54324))
{
	BattleService.SetCallback(std::bind(&MyBattle::MatchTransfer, this, std::placeholders::_1, std::placeholders::_2));
}

MyBattle::~MyBattle()
{
}

void MyBattle::Open()
{
	redis.Connect(ConfigParser::GetString("SessionAddr"), ConfigParser::GetInt("SessionPort", 6379));
	Logger::log("Battle Server Start as " + self_keyword, Logger::LogType::info);
}

void MyBattle::Close()
{
	gamepool.Stop();
	BattleService.Close();
	redis.Close();
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
	auto session = this->sessions.FindRKey(cookie);
	if(!session)	//세션이 match로부터 오지 않는 경우
	{
		Logger::log("Account " + std::to_string(account_id) + " Failed to Authenticate : No Matching Game", Logger::LogType::auth);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		redis.SetInfo(account_id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.
		client->Close();
		return;
	}
	std::shared_ptr<MyGame> GameSession = session->second;

	//Game에 접속 보내기.
	int side = GameSession->Connect(account_id, client);	//여기에서 Dup-Access를 처리한다.
	if(side < 0)	//게임이 종료되었거나 올바르지 않은 세션이 올라온 경우
	{
		Logger::log("Account " + std::to_string(account_id) + " Failed to Authenticate : Wrong Game Found", Logger::LogType::auth);
		this->sessions.EraseLKey(account_id);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		redis.SetInfo(account_id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.
		client->Close();
		return;
	}

	//사용자에게 정보 전달.
	{
		ByteQueue info = ByteQueue::Create<byte>(GAME_PLAYER_INFO_NAME);
		if(GameSession->players[0].id == account_id)
			info += GameSession->players[0].info + GameSession->players[1].info;
		else
			info += GameSession->players[1].info + GameSession->players[0].info;
		client->Send(info);
	}

	Logger::log("Account " + std::to_string(account_id) + " logged in from " + client->ToString(), Logger::LogType::auth);
	ClientProcess(client, account_id, side, GameSession);
	client->SetTimeout(SESSION_TIMEOUT / 2, std::bind(&MyBattle::SessionProcess, this, std::placeholders::_1, account_id, cookie));
}

void MyBattle::ClientProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, int side, std::shared_ptr<MyGame> GameSession)
{
	target_client->StartRecv([this, account_id, side, GameSession](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		if(!recv_ec)
		{
			GameSession->Disconnect(side, client);
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

		if(!client->Send(ByteQueue::Create<byte>(ANS_HEARTBEAT)) ||
			!redis.RefreshInfo(account_id, cookie, self_keyword))
			client->Close();
		else
			SessionProcess(client, account_id, cookie);
	});
}

void MyBattle::GameProcess(std::shared_ptr<boost::asio::steady_timer> timer, std::shared_ptr<MyGame> game, const boost::system::error_code &error_code)
{
	if(!gamepool.is_open())
		return;
	std::exception_ptr eptr = nullptr;

	try
	{
		if(game->Work(error_code.failed()))
		{
			timer->async_wait(std::bind(&MyBattle::GameProcess, this, timer, game, std::placeholders::_1));
			return;
		}

		std::queue<MyGame::player_info> players;
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			players.push(game->players[i]);

		while(!players.empty())
		{
			auto player = players.front();
			players.pop();
			auto erase_result = sessions.EraseLKey(player.id);
			if(!erase_result)	//session이 없으면 이미 빠진 것이므로 무시.
				continue;
			auto [id, cookie, _g] = *erase_result;
			auto socket = player.socket;
			if(!socket)		//소켓이 nullptr이면 게임 중간에 나간 것이므로 무시.
			{
				redis.ClearInfo(id, cookie, self_keyword);	//단, 이미 나갔기 때문에 세션은 지운다.
				continue;
			}
			redis.SetInfo(id, cookie, keyword_match);	//battle서버에 있을 필요 없는 세션은 match서버로 보낸다.

			ByteQueue packet = ByteQueue::Create<byte>(player.result);
			socket->Send(packet);
			socket->Close();
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
		sessions.EraseLKey(game->players[i].id);	//exception 발생할 시를 대비한 쿠키 지우기. 이미 예외가 발생했으므로, 후처리는 못한다고 봐야 한다.

	if(eptr)
		std::rethrow_exception(eptr);
}

void MyBattle::MatchTransfer(const Account_ID_t& lpid, const Account_ID_t& rpid)
{
	if(lpid <= 0 || rpid <= 0)
	{
		BattleService.SetUsage(sessions.Size());
		return;
	}

	auto timer = gamepool.GetTimer();
	if(timer)
	{
		auto lpresult = redis.GetInfoFromID(lpid);
		auto rpresult = redis.GetInfoFromID(rpid);
		if(lpresult && rpresult)
		{
			Hash_t lpcookie = lpresult->first;
			Hash_t rpcookie = rpresult->first;
			std::shared_ptr<MyGame> new_game = std::make_shared<MyGame>(lpid, rpid, timer, &dbpool);

			sessions.Insert(lpid, lpcookie, new_game);
			sessions.Insert(rpid, rpcookie, new_game);
			GameProcess(timer, new_game, boost::system::error_code());	//최초 Timer 등록
		}
	}
	BattleService.SetUsage(sessions.Size());
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