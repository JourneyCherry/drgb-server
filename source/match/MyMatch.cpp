#include "MyMatch.hpp"

MyMatch::MyMatch() : 
	connectee(this), 
	connector_battle(this, MyConfigParser::GetString("Battle1_Addr"), MyConfigParser::GetInt("Battle1_Port"), "match"),
	t_matchmaker(std::bind(&MyMatch::MatchMake, this, std::placeholders::_1), this),
	MyServer(MyConfigParser::GetInt("Match1_ClientPort_Web", 54321), MyConfigParser::GetInt("Match1_ClientPort_TCP", 54322))
{

}

MyMatch::~MyMatch()
{
}

void MyMatch::Open()
{
	MyPostgres::Open();
	connectee.Open(MyConfigParser::GetInt("Match1_Port"));
	connectee.Accept("auth", std::bind(&MyMatch::MatchInquiry, this, std::placeholders::_1));
	connectee.Accept("battle", std::bind(&MyMatch::MatchInquiry, this, std::placeholders::_1));
	connector_battle.Connect();
	MyLogger::log("Match Server Start", MyLogger::LogType::info);
}

void MyMatch::Close()
{
	connectee.Close();
	connector_battle.Disconnect();
	MyPostgres::Close();
	MyLogger::log("Match Server Stop", MyLogger::LogType::info);
}

void MyMatch::ClientProcess(std::shared_ptr<MyClientSocket> client)
{
	Account_ID_t account_id = 0;
	std::shared_ptr<MyNotifier> noti = nullptr;
	// Authentication
	{
		auto answer = client->Recv();
		if(!answer)
		{
			client->Close();
			if(answer.error() < 0)
				return;
			throw MyExcepts("client::recv() error : " + std::to_string(answer.error()), __STACKINFO__);
		}
		Hash_t cookie = answer->pop<Hash_t>();
		auto result = sessions.FindRKey(cookie);
		if(result)
		{
			account_id = result->first;
			noti = result->second;
		}
		if(account_id <= 0)
		{
			client->Send(MyBytes::Create<byte>(ERR_NO_MATCH_ACCOUNT));
			client->Close();
			return;
		}
	}

	noti = std::make_shared<MyNotifier>();
	sessions.InsertLKeyValue(account_id, noti);
	mylib::threads::Thread receiver([&](std::shared_ptr<bool> killswitch)
	{
		while(!(*killswitch))
		{
			auto result = client->Recv();
			if(!result)
			{
				noti->push(std::make_shared<MyDisconnectMessage>());
				break;
			}
			noti->push(std::make_shared<MyClientMessage>(*result));
		}
	});

	bool isAnswered = false;
	while(isRunning && !isAnswered)
	{
		auto message = noti->wait();
		if(!message)
			break;
		
		switch((*message)->Type())
		{
			case MyClientMessage::Message_ID:
				{
					MyBytes data = ((MyClientMessage*)message->get())->message;
					//TODO : 클라이언트 메시지 처리
					byte header = data.pop<byte>();
					switch(header)
					{
						case REQ_STARTMATCH:	//매치메이커 큐에 넣기.
							try
							{
								int win = 0;
								int loose = 0;
								int draw = 0;
								MyPostgres db;
								auto result = db.exec1("SELECT win_count, loose_count, draw_count FROM userlist WHERE id=" + std::to_string(account_id));
								win = result[0].as<int>();
								loose = result[1].as<int>();
								draw = result[2].as<int>();
								matchmaker.Enter(account_id, win, draw, loose);
							}
							catch(const pqxx::pqxx_exception & e)
							{
								client->Send(MyBytes::Create<byte>(ERR_DB_FAILED));
								client->Close();
								throw MyExcepts("DB Failed : " + std::string(e.base().what()), __STACKINFO__);
							}
							catch(const std::exception &e)
							{
								client->Send(MyBytes::Create<byte>(ERR_DB_FAILED));
								client->Close();
								throw MyExcepts("DB Failed : " + std::string(e.what()), __STACKINFO__);
							}
							break;
						case REQ_PAUSEMATCH:	//매치메이커 큐에서 빼기.
							matchmaker.Exit(account_id);
							break;
						default:
							client->Send(MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION));
							break;
					}
				}
				break;
			case MyDisconnectMessage::Message_ID:
				matchmaker.Exit(account_id);
				sessions.EraseLKey(account_id);
				isAnswered = true;
				break;
			case MyMatchMakerMessage::Message_ID:
				{
					int battle_id = ((MyMatchMakerMessage*)message->get())->BattleServer;
					MyBytes answer = MyBytes::Create<byte>(ANS_MATCHMADE);
					answer.push<int>(battle_id);
					client->Send(answer);
					isAnswered = true;
				}
				break;
			default:
				client->Send(MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION));
				break;
		}
	}
	client->Close();
	receiver.stop();
}

MyBytes MyMatch::MatchInquiry(MyBytes bytes)
{
	byte header = bytes.pop<byte>();
	switch(header)
	{
		case INQ_ACCOUNT_CHECK:
			{
				Account_ID_t account_id = bytes.pop<Account_ID_t>();

				//저장된 세션(쿠키)가 있는지 확인.
				{
					auto cookie = sessions.FindLKey(account_id);
					if(cookie)
					{
						MyBytes answer = MyBytes::Create<byte>(ERR_EXIST_ACCOUNT_MATCH);
						answer.push<Hash_t>(cookie->first);
						answer.push<Seed_t>(MACHINE_ID);
						return answer;
					}
				}

				//battle 서버에 요청하여 cookie 찾기.	//TODO : 추후, battle 서버가 늘어날 경우, 모든 battle서버에 요청하도록 변경 필요.
				{
					MyBytes query = MyBytes::Create<byte>(INQ_ACCOUNT_CHECK);
					query.push<Account_ID_t>(account_id);
					MyBytes answer = connector_battle.Request(query);
					byte query_header = answer.pop<byte>();
					switch(query_header)
					{
						case ERR_NO_MATCH_ACCOUNT:
							return MyBytes::Create<byte>(ERR_NO_MATCH_ACCOUNT);
						case ERR_EXIST_ACCOUNT_BATTLE:	//쿠키와 몇번 배틀서버인지(unsigned int)는 뒤에 붙어있다.
						case ERR_OUT_OF_CAPACITY:		//서버 접속 불가. 단일 byte.
							return answer;
					}
				}
			}
			break;
		case INQ_COOKIE_TRANSFER:
			{
				Account_ID_t account_id = bytes.pop<Account_ID_t>();
				Hash_t cookie = bytes.pop<Hash_t>();

				if(sessions.Size() >= MAX_CLIENTS)
					return MyBytes::Create<byte>(ERR_OUT_OF_CAPACITY);
				
				if(!sessions.Insert(account_id, cookie, nullptr))
					return MyBytes::Create<byte>(ERR_EXIST_ACCOUNT_MATCH);
			}
			return MyBytes::Create<byte>(SUCCESS);
	}

	return MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION);
}

void MyMatch::MatchMake(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "MatchMaker");
#endif
	while(isRunning && !(*killswitch))
	{
		std::this_thread::sleep_for(std::chrono::seconds(rematch_delay));
		matchmaker.Process();

		while(matchmaker.isThereMatch())
		{
			auto [lp, rp] = matchmaker.GetMatch();
			Account_ID_t lpid = lp.whoami();
			Account_ID_t rpid = rp.whoami();
			auto lpresult = sessions.FindLKey(lpid);
			auto rpresult = sessions.FindLKey(rpid);
			Hash_t lpcookie;
			Hash_t rpcookie;
			std::shared_ptr<MyNotifier> lpnotifier;
			std::shared_ptr<MyNotifier> rpnotifier;
			if(lpresult)
			{
				lpcookie = lpresult->first;
				lpnotifier = lpresult->second;
			}
			if(rpresult)
			{
				rpcookie = rpresult->first;
				rpnotifier = rpresult->second;
			}
			
			if(lpresult && rpresult)	//둘다 정상접속 상태이면
			{
				//battle서버에 battle 등록.
				MyBytes req = MyBytes::Create<byte>(INQ_MATCH_TRANSFER);
				req.push<Account_ID_t>(lpid);
				req.push<Hash_t>(sessions.FindLKey(lpid)->first);
				req.push<Account_ID_t>(rpid);
				req.push<Hash_t>(sessions.FindLKey(rpid)->first);
				
				{
					int battle_server = 1;	//TODO : 모든 Battle 서버에 쿼리 날려보기. battle서버가 늘어날 경우, 해당 서버 번호를 넣어야 한다.
					MyBytes ans = connector_battle.Request(req);
					byte result = ans.pop<byte>();
					if(result == SUCCESS)
					{
						lpnotifier->push(std::make_shared<MyMatchMakerMessage>(battle_server));
						rpnotifier->push(std::make_shared<MyMatchMakerMessage>(battle_server));
						matchmaker.PopMatch();

						sessions.EraseLKey(lpid);
						sessions.EraseLKey(rpid);
					}
					else	//battle서버에서 매치 전달에 실패한 경우
					{
						MyLogger::raise(std::make_exception_ptr(MyExcepts("Battle Server Cannot Accept Battle : " + std::to_string(result), __STACKINFO__)));
						break;	//TODO : 다른 가용 battle서버를 찾아야 한다.
					}
				}
			}
			else	//매칭된 둘 중에 한명이라도 이미 접속을 종료했다면 남은 한명을 다시 큐에 집어넣는다.
			{
				if(lpresult > 0) matchmaker.Enter(lp);
				if(rpresult > 0) matchmaker.Enter(rp);
				matchmaker.PopMatch();
			}
		}
	}
}