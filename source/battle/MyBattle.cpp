#include "MyBattle.hpp"

MyBattle::MyBattle() : 
	connectee(this), 
	connector_match(this, ConfigParser::GetString("Match1_Addr"), ConfigParser::GetInt("Match1_Port", 52431), "battle"), 
	gamepool("GamePool", std::bind(&MyBattle::GameProcess, this, std::placeholders::_1), this),
	MyServer(ConfigParser::GetInt("Battle1_ClientPort_Web", 54321), ConfigParser::GetInt("Battle1_ClientPort_TCP", 54322))
{
}

MyBattle::~MyBattle()
{
}

void MyBattle::Open()
{
	MyPostgres::Open();
	connectee.Open(ConfigParser::GetInt("Battle1_Port"));
	connectee.Accept("match", std::bind(&MyBattle::BattleInquiry, this, std::placeholders::_1));
	connector_match.Connect();
	Logger::log("Battle Server Start", Logger::LogType::info);
}

void MyBattle::Close()
{
	connectee.Close();
	gamepool.stop();
	connector_match.Disconnect();
	MyPostgres::Close();
	Logger::log("Battle Server Stop", Logger::LogType::info);
}

void MyBattle::ClientProcess(std::shared_ptr<MyClientSocket> client)
{
	auto authenticate = client->Recv();
	if(!authenticate)
	{
		client->Close();
		return;
	}
	Hash_t cookie = authenticate->pop<Hash_t>();
	auto session = cookies.FindRKey(cookie);
	if(!session)	//세션이 match로부터 오지 않는 경우
	{
		client->Close();
		return;
	}

	Account_ID_t account_id = session->first;
	std::shared_ptr<MyGame> GameSession = session->second;

	//Game에 접속 보내기.
	int side = GameSession->Connect(account_id, client);
	if(side < 0)	//게임이 종료되었거나 올바르지 않은 세션이 올라온 경우
	{
		cookies.EraseLKey(account_id);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
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

	ErrorCode ec;
	while(isRunning && ec)
	{
		auto msg = client->Recv();
		if(!msg)
		{
			//Game에 disconnect 메시지 보내기.
			GameSession->Disconnect(side, client);
			ec = msg.error();
			break;
		}
		byte header = msg->pop<byte>();
		switch(header)
		{
			case REQ_GAME_ACTION:
				try
				{
					int action = msg->pop<int>();
					GameSession->Action(side, action);
				}
				catch(const std::exception &e)
				{
					ec = client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
					Logger::log(e.what(), Logger::LogType::error);
				}
				break;
			default:
				ec = client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
				break;
		}
	}

	client->Close();
	if(!client->isNormalClose(ec))
		throw ErrorCodeExcept(ec, __STACKINFO__);
}

ByteQueue MyBattle::BattleInquiry(ByteQueue bytes)
{
	byte header = bytes.pop<byte>();
	switch(header)
	{
		case INQ_ACCOUNT_CHECK:
			{
				Account_ID_t account_id = bytes.pop<Account_ID_t>();
				auto cookie = cookies.FindLKey(account_id);
				if(!cookie)
					return ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT);
				ByteQueue result = ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_BATTLE);
				result.push<Hash_t>(cookie->first);
				result.push<Seed_t>(MACHINE_ID);
				return result;
			}
			break;
		case INQ_AVAILABLE:
			{
				int capacity = gamepool.remain();
				if(capacity == 0)
					return ByteQueue::Create<byte>(ERR_OUT_OF_CAPACITY);
					
				ByteQueue answer = ByteQueue::Create<byte>(SUCCESS);
				answer.push<int>(capacity);
				return answer;
			}
			break;
		case INQ_MATCH_TRANSFER:
			{
				int capacity = gamepool.remain();
				if(capacity == 0)
					return ByteQueue::Create<byte>(ERR_OUT_OF_CAPACITY);

				Account_ID_t lpid = bytes.pop<Account_ID_t>();
				Hash_t lpcookie = bytes.pop<Hash_t>();
				Account_ID_t rpid = bytes.pop<Account_ID_t>();
				Hash_t rpcookie = bytes.pop<Hash_t>();

				std::shared_ptr<MyGame> new_game = std::make_shared<MyGame>(lpid, rpid);

				cookies.Insert(lpid, lpcookie, new_game);
				cookies.Insert(rpid, rpcookie, new_game);
				gamepool.insert(new_game);

				return ByteQueue::Create<byte>(SUCCESS);
			}
			break;
	}
	return ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
}

void MyBattle::GameProcess(std::shared_ptr<MyGame> game)
{
	std::array<Account_ID_t, MyGame::MAX_PLAYER> ids;
	for(int i = 0;i<MyGame::MAX_PLAYER;i++)
		ids[i] = game->players[i].id;
	std::exception_ptr eptr = nullptr;

	try
	{
		game->Work();

		std::queue<MyGame::player_info> players;
		for(int i = 0;i<MyGame::MAX_PLAYER;i++)
			players.push(game->players[i]);

		while(!players.empty())
		{
			Seed_t match_server_seed = 1;	//TODO : match server load balancing 하기.

			auto player = players.front();
			players.pop();
			auto erase_result = cookies.EraseLKey(player.id);
			if(!erase_result)	//session이 없으면 이미 빠진 것이므로 무시.
				continue;
			auto [id, cookie, _g] = *erase_result;
			auto socket = player.socket;
			if(!socket)		//소켓이 nullptr이면 게임 중간에 나간 것이므로 무시.
				continue;

			ByteQueue req = ByteQueue::Create<byte>(INQ_COOKIE_TRANSFER);
			req.push<Account_ID_t>(id);
			req.push<Hash_t>(cookie);

			ByteQueue msg = connector_match.Request(req);
			byte header = msg.pop<byte>();

			switch(header)
			{
				case SUCCESS:
				case ERR_EXIST_ACCOUNT_MATCH:	//이미 해당 쿠키가 Match서버에 있으면 Match 서버에 주도권을 준다.(즉, 정상동작)
					break;
				default:
					Logger::log("Cookie Transfer Failed : " + std::to_string(header), Logger::LogType::error);
					match_server_seed = -1;
					break;
			}

			ByteQueue packet = ByteQueue::Create<byte>(player.result);
			packet.push<Seed_t>(match_server_seed);
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

	for(Account_ID_t id : ids)	//exception 발생할 시를 대비한 쿠키 지우기. 이미 예외가 발생했으므로, 후처리는 못한다고 봐야 한다.
		cookies.EraseLKey(id);

	if(eptr)
		std::rethrow_exception(eptr);
}