#include "MyBattle.hpp"

MyBattle::MyBattle() : 
	connectee(this), 
	connector_match(this, MyConfigParser::GetString("Match1_Addr"), MyConfigParser::GetInt("Match1_Port", 52431), "battle"), 
	gamepool(MAX_GAME), 
	poolManager(std::bind(&MyBattle::pool_manage, this, std::placeholders::_1), this, false), 
	MyServer(MyConfigParser::GetInt("Battle1_ClientPort_Web", 54321), MyConfigParser::GetInt("Battle1_ClientPort_TCP", 54322))
{
}

MyBattle::~MyBattle()
{
}

void MyBattle::Open()
{
	MyPostgres::Open();
	connectee.Open(MyConfigParser::GetInt("Battle1_Port"));
	connectee.Accept("match", std::bind(&MyBattle::BattleInquiry, this, std::placeholders::_1));
	connector_match.Connect();
	poolManager.start();
	MyLogger::log("Battle Server Start", MyLogger::LogType::info);
}

void MyBattle::Close()
{
	gamepool.Stop();
	poolManager.stop();
	connector_match.Disconnect();
	connectee.Close();
	MyPostgres::Close();
	MyLogger::log("Battle Server Stop", MyLogger::LogType::info);
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
		client->Send(MyBytes::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		client->Close();
		return;
	}

	while(isRunning)
	{
		auto msg = client->Recv();
		if(!msg)
		{
			//Game에 disconnect 메시지 보내기.
			GameSession->Disconnect(side, client);
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
					client->Send(MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION));
					MyLogger::raise();
				}
				break;
			default:
				client->Send(MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION));
				break;
		}
	}

	client->Close();
}

MyBytes MyBattle::BattleInquiry(MyBytes bytes)
{
	byte header = bytes.pop<byte>();
	switch(header)
	{
		case INQ_ACCOUNT_CHECK:
			{
				Account_ID_t account_id = bytes.pop<Account_ID_t>();
				auto cookie = cookies.FindLKey(account_id);
				if(!cookie)
					return MyBytes::Create<byte>(ERR_NO_MATCH_ACCOUNT);
				MyBytes result = MyBytes::Create<byte>(ERR_EXIST_ACCOUNT_BATTLE);
				result.push<Hash_t>(cookie->first);
				result.push<Seed_t>(MACHINE_ID);
				return result;
			}
			break;
		case INQ_AVAILABLE:
			{
				int now_game = gamepool.size();
				if(now_game >= MAX_GAME)
					return MyBytes::Create<byte>(ERR_OUT_OF_CAPACITY);
					
				MyBytes answer = MyBytes::Create<byte>(SUCCESS);
				answer.push<int>(MAX_GAME - now_game);
				return answer;
			}
			break;
		case INQ_MATCH_TRANSFER:
			{
				int now_game = gamepool.size();
				if(now_game >= MAX_GAME)
					return MyBytes::Create<byte>(ERR_OUT_OF_CAPACITY);

				Account_ID_t lpid = bytes.pop<Account_ID_t>();
				Hash_t lpcookie = bytes.pop<Hash_t>();
				Account_ID_t rpid = bytes.pop<Account_ID_t>();
				Hash_t rpcookie = bytes.pop<Hash_t>();

				std::shared_ptr<MyGame> new_game = std::make_shared<MyGame>(lpid, rpid);

				cookies.Insert(lpid, lpcookie, new_game);
				cookies.Insert(rpid, rpcookie, new_game);
				gamepool.insert(new_game);

				return MyBytes::Create<byte>(SUCCESS);
			}
			break;
	}
	return MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION);
}

void MyBattle::pool_manage(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "PoolManager");
#endif
	while(!(*killswitch))
	{
		try
		{
			auto result = gamepool.WaitForFinish();
			if(result == nullptr)	//null이 오면 gamepool이 멈춤 + queue가 비어있다는 소리이므로 종료 필요.
				break;

			MyGame *game = dynamic_cast<MyGame*>(result.get());	//바로 윗줄의 result가 shared_ptr의 형태이므로, 소유권을 여기서 가질 수 있다.
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

				MyBytes req = MyBytes::Create<byte>(INQ_COOKIE_TRANSFER);
				req.push<Account_ID_t>(id);
				req.push<Hash_t>(cookie);

				MyBytes msg = connector_match.Request(req);
				byte header = msg.pop<byte>();

				switch(header)
				{
					case SUCCESS:
					case ERR_EXIST_ACCOUNT_MATCH:	//이미 해당 쿠키가 Match서버에 있으면 Match 서버에 주도권을 준다.(즉, 정상동작)
						break;
					default:
						MyLogger::log("Cookie Transfer Failed : " + std::to_string(header), MyLogger::LogType::error);
						match_server_seed = -1;
						break;
				}

				MyBytes packet = MyBytes::Create<byte>(player.result);
				packet.push<Seed_t>(match_server_seed);
				socket->Send(packet);
				socket->Close();
			}
		}
		catch(const std::exception& e)
		{
			MyLogger::raise();
		}
		//TODO : 중간에 exception이 발생해도 session들을 빼내는 작업이 필요하다.
	}
}