#include "MyBattle.hpp"

MyBattle::MyBattle() : connectee(this), gamepool(MAX_GAME), poolManager(std::bind(&MyBattle::pool_manage, this, std::placeholders::_1), this, false), MyServer(MyConfigParser::GetInt("Battle1_ClientPort_Web", 54321), MyConfigParser::GetInt("Battle1_ClientPort_TCP", 54322))
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
	poolManager.start();
	MyLogger::log("Battle Server Start", MyLogger::LogType::info);
}

void MyBattle::Close()
{
	poolManager.stop();
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
	if(!session)
	{
		client->Close();
		return;
	}

	Account_ID_t account_id = session->first;
	std::shared_ptr<MyGame> GameSession = session->second;

	//Game에 접속 보내기.
	int side = GameSession->Connect(account_id, client);

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
				result.push<unsigned int>(MACHINE_ID);
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
				Account_ID_t lpid = bytes.pop<Account_ID_t>();
				Hash_t lpcookie = bytes.pop<Hash_t>();
				Account_ID_t rpid = bytes.pop<Account_ID_t>();
				Hash_t rpcookie = bytes.pop<Hash_t>();

				std::shared_ptr<MyGame> new_game = std::make_shared<MyGame>(lpid, rpid);

				cookies.Insert(lpid, lpcookie, new_game);
				cookies.Insert(rpid, rpcookie, new_game);
				gamepool.insert(new_game);
			}
			break;
	}
	return MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION);
}

void MyBattle::pool_manage(std::shared_ptr<bool> killswitch)
{
	while(!(*killswitch))
	{
		try
		{
			auto result = gamepool.WaitForFinish();
		}
		catch(const std::exception& e)
		{
			MyLogger::raise();
		}
	}
}