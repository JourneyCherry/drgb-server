#include "MyBattle.hpp"

MyBattle::MyBattle() : 
	connector("battle", 1, this), keyword_match("match"),
	gamepool("GamePool", std::bind(&MyBattle::GameProcess, this, std::placeholders::_1), this),
	MyServer(ConfigParser::GetInt("Battle1_ClientPort_Web", 54324), ConfigParser::GetInt("Battle1_ClientPort_TCP", 54424))
{
}

MyBattle::~MyBattle()
{
}

void MyBattle::Open()
{
	MyPostgres::Open();

	connector.Connect(ConfigParser::GetString("Match_Addr"), ConfigParser::GetInt("Match_Port", 52431), keyword_match, std::bind(&MyBattle::BattleInquiry, this, std::placeholders::_1));
	Logger::log("Battle Server Start", Logger::LogType::info);
}

void MyBattle::Close()
{
	gamepool.stop();
	connector.Close();
	MyPostgres::Close();
	Logger::log("Battle Server Stop", Logger::LogType::info);
}

void MyBattle::AcceptProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec)
		return;

	client->KeyExchange(std::bind(&MyBattle::EnterProcess, this, std::placeholders::_1, std::placeholders::_2));
}

void MyBattle::EnterProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec)
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : " + ec.message_code(), Logger::LogType::auth);
		client->Close();
		return;
	}

	client->StartRecv(std::bind(&MyBattle::AuthenticateProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	client->SetTimeout(TIME_AUTHENTICATE);
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
	auto session = this->cookies.FindRKey(cookie);
	if(!session)	//세션이 match로부터 오지 않는 경우
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : No matching Cookie", Logger::LogType::auth);
		client->Close();
		return;
	}
	Account_ID_t account_id = session->first;
	std::shared_ptr<MyGame> GameSession = session->second;

	//Game에 접속 보내기.
	int side = GameSession->Connect(account_id, client);
	if(side < 0)	//게임이 종료되었거나 올바르지 않은 세션이 올라온 경우
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : No matching Game", Logger::LogType::auth);
		this->cookies.EraseLKey(account_id);
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

	client->StartRecv(std::bind(&MyBattle::ClientProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, account_id, side, GameSession));
	Logger::log("Account " + std::to_string(account_id) + " logged in from " + client->ToString(), Logger::LogType::auth);
}

void MyBattle::ClientProcess(std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec, Account_ID_t account_id, int side, std::shared_ptr<MyGame> GameSession)
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

				auto answer = ByteQueue::Create<byte>(SUCCESS);
				answer.push<Seed_t>(MACHINE_ID);
				return answer;
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
			//Seed_t match_server_seed = 1;	//TODO : 추후, Match Server가 distributed되면 Load Balancing 하도록 변경 필요

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

			auto req_result = connector.Request(keyword_match, req);
			if(req_result)
			{
				byte header = req_result->pop<byte>();

				switch(header)
				{
					case SUCCESS:
					case ERR_EXIST_ACCOUNT_MATCH:	//이미 해당 쿠키가 Match서버에 있으면 Match 서버에 주도권을 준다.(즉, 정상동작)
						break;
					default:
						Logger::log("Cookie Transfer Failed : " + ErrorCode(header).message_code(), Logger::LogType::error);
						break;
				}
			}
			else
			{
				Logger::log("Cookie Transfer Failed : " + req_result.error().message_code(), Logger::LogType::error);
			}

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

	for(Account_ID_t id : ids)	//exception 발생할 시를 대비한 쿠키 지우기. 이미 예외가 발생했으므로, 후처리는 못한다고 봐야 한다.
		cookies.EraseLKey(id);

	if(eptr)
		std::rethrow_exception(eptr);
}