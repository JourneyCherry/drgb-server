#include "MyMatch.hpp"

MyMatch::MyMatch() : 
	connectee("match", ConfigParser::GetInt("Match1_Port"), this), 
	t_matchmaker(std::bind(&MyMatch::MatchMake, this), this),
	MyServer(ConfigParser::GetInt("Match1_ClientPort_Web", 54321), ConfigParser::GetInt("Match1_ClientPort_TCP", 54322))
{
}

MyMatch::~MyMatch()
{
}

void MyMatch::Open()
{
	MyPostgres::Open();
	connectee.Accept("auth", std::bind(&MyMatch::MatchInquiry, this, std::placeholders::_1));
	connectee.Accept("battle", std::bind(&MyMatch::MatchInquiry, this, std::placeholders::_1));
	connectee.Open();
	Logger::log("Match Server Start", Logger::LogType::info);
}

void MyMatch::Close()
{
	connectee.Close();
	MyPostgres::Close();
	Logger::log("Match Server Stop", Logger::LogType::info);
}

void MyMatch::ClientProcess(std::shared_ptr<MyClientSocket> client)
{
	StackErrorCode sec = client->KeyExchange();
	if(!sec)
	{
		client->Close();
		Logger::log("Client " + client->ToString() + " Failed to KeyExchange", Logger::LogType::auth);
		if(!MyClientSocket::isNormalClose(sec))
			throw ErrorCodeExcept(sec, __STACKINFO__);
	}
	Account_ID_t account_id = 0;
	std::shared_ptr<Notifier> noti = nullptr;
	// Authentication
	{
		auto answer = client->Recv(TIME_AUTHENTICATE);
		if(!answer)
		{
			client->Close();
			return;
		}
		Hash_t cookie = answer->pop<Hash_t>();
		auto result = sessions.FindRKey(cookie);
		if(result)
		{
			account_id = result->first;
			noti = result->second;
			if(noti != nullptr)
				noti->push(std::make_shared<MyDupAccessMessage>(client->ToString()));
		}
		if(account_id <= 0)
		{
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
			client->Close();
			Logger::log("Client " + client->ToString() + " Failed to Authenticate", Logger::LogType::auth);
			return;
		}
	}

	noti = std::make_shared<Notifier>();
	sessions.InsertLKeyValue(account_id, noti);
	Thread receiver([&]()
	{
		Expected<ByteQueue, ErrorCode> result{ByteQueue()};
		while(result = client->Recv())
			noti->push(std::make_shared<MyClientMessage>(*result));

		noti->push(std::make_shared<MyDisconnectMessage>());
		if(!MyClientSocket::isNormalClose(result.error()))
			throw ErrorCodeExcept(result.error(), __STACKINFO__);
	});

	Logger::log("Account " + std::to_string(account_id) + " logged in from " + client->ToString(), Logger::LogType::auth);

	//사용자에게 정보 전달.
	try
	{
		MyPostgres db;
		ByteQueue infopacket = ByteQueue::Create<byte>(GAME_PLAYER_INFO);
		auto [nickname, win, draw, loose] = db.GetInfo(account_id);
		infopacket.push<Achievement_ID_t>(nickname);	//nickname
		infopacket.push<int>(win);	//win
		infopacket.push<int>(draw);	//draw
		infopacket.push<int>(loose);	//loose

		auto result = db.GetAllAchieve(account_id);
		for(auto pair : result)
		{
			infopacket.push<Achievement_ID_t>(pair.first);	//achieve id
			infopacket.push<int>(pair.second);	//achieve count
		}
		sec = StackErrorCode(client->Send(infopacket), __STACKINFO__);
	}
	catch(const pqxx::pqxx_exception & e)
	{
		sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
		client->Close();
		throw StackTraceExcept("DB Failed : " + std::string(e.base().what()), __STACKINFO__);
	}
	catch(const std::exception &e)
	{
		sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
		client->Close();
		throw StackTraceExcept("DB Failed : " + std::string(e.what()), __STACKINFO__);
	}

	bool isAnswered = false;
	while(isRunning && !isAnswered && sec)
	{
		auto message = noti->wait();
		if(!message)
			break;
		
		switch((*message)->Type())
		{
			case MyClientMessage::Message_ID:
				{
					ByteQueue data = ((MyClientMessage*)message->get())->message;
					//클라이언트 메시지 처리
					byte header = data.pop<byte>();
					switch(header)
					{
						case REQ_CHPWD:
							try
							{
								Pwd_Hash_t old_pwd = data.pop<Pwd_Hash_t>();
								Pwd_Hash_t new_pwd = data.pop<Pwd_Hash_t>();
								MyPostgres db;
								if(db.ChangePwd(account_id, old_pwd, new_pwd))
								{
									db.commit();
									sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(SUCCESS)), __STACKINFO__);
								}
								else
								{
									sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT)), __STACKINFO__);
								}
							}
							catch(const pqxx::pqxx_exception & e)
							{
								sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
								Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::debug);
							}
							break;
						case REQ_STARTMATCH:	//매치메이커 큐에 넣기.
							try
							{
								MyPostgres db;
								auto [_, win, draw, loose] = db.GetInfo(account_id);
								matchmaker.Enter(account_id, win, draw, loose);
							}
							catch(const pqxx::pqxx_exception & e)
							{
								sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
								Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::debug);
							}
							catch(const std::exception &e)
							{
								sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
								Logger::log("DB Failed : " + std::string(e.what()), Logger::LogType::debug);
							}
							break;
						case REQ_PAUSEMATCH:	//매치메이커 큐에서 빼기.
							matchmaker.Exit(account_id);
							break;
						case REQ_CHNAME:
							try
							{
								Achievement_ID_t nameindex = data.pop<Achievement_ID_t>();
								MyPostgres db;
								db.SetNickName(account_id, nameindex);	//성공하든, 실패하든 항상 정보를 다시 던져준다.

								ByteQueue infopacket = ByteQueue::Create<byte>(GAME_PLAYER_INFO);
								auto [nickname, win, draw, loose] = db.GetInfo(account_id);
								infopacket.push<Achievement_ID_t>(nickname);
								infopacket.push<int>(win);
								infopacket.push<int>(draw);	
								infopacket.push<int>(loose);

								db.commit();
								client->Send(infopacket);
							}
							catch(const pqxx::pqxx_exception & e)
							{
								sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
								client->Close();
								throw StackTraceExcept("DB Failed : " + std::string(e.base().what()), __STACKINFO__);
							}
							catch(const std::exception &e)
							{
								sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__);
								client->Close();
								throw StackTraceExcept("DB Failed : " + std::string(e.what()), __STACKINFO__);
							}
							break;
						default:
							sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION)), __STACKINFO__);
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
					ByteQueue answer = ByteQueue::Create<byte>(ANS_MATCHMADE);
					answer.push<int>(battle_id);
					sec = StackErrorCode(client->Send(answer), __STACKINFO__);
					isAnswered = true;
				}
				break;
			case MyDupAccessMessage::Message_ID:
				{
					matchmaker.Exit(account_id);
					std::string access_addr = ((MyDupAccessMessage*)message->get())->access_addr;
					Logger::log("Account " + std::to_string(account_id) + " has dup access : " + client->ToString() + " -> " + access_addr, Logger::LogType::auth);
					sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_DUPLICATED_ACCESS)), __STACKINFO__);
					isAnswered = true;
				}
				break;
			default:
				sec = StackErrorCode(client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION)), __STACKINFO__);
				break;
		}
	}
	client->Close();
	receiver.join();
	Logger::log("Account " + std::to_string(account_id) + " logged out from " + client->ToString(), Logger::LogType::auth);
	if(!MyClientSocket::isNormalClose(sec))
		throw ErrorCodeExcept(sec, __STACKINFO__);
}

ByteQueue MyMatch::MatchInquiry(ByteQueue bytes)
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
						ByteQueue answer = ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_MATCH);
						answer.push<Hash_t>(cookie->first);
						return answer;
					}
				}

				//battle 서버에 요청하여 cookie 찾기.
				{
					ByteQueue query = ByteQueue::Create<byte>(INQ_ACCOUNT_CHECK);
					query.push<Account_ID_t>(account_id);
					Hash_t cookie;
					Seed_t battle_id = -1;
					connectee.Request("battle", query, [&](std::shared_ptr<MyConnector> connector, ByteQueue answer)
					{
						if(answer.pop<byte>() == ERR_EXIST_ACCOUNT_BATTLE)	//ERR_EXIST_ACCOUNT_BATTLE을 주는 클라이언트 찾기.
						{
							cookie = answer.pop<Hash_t>();
							battle_id = answer.pop<Seed_t>();
						}
					});
					if(battle_id < 0)
						return ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT);
					ByteQueue answer = ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_BATTLE);
					answer.push<Hash_t>(cookie);
					answer.push<Seed_t>(battle_id);
					return answer;
				}
			}
			break;
		case INQ_COOKIE_TRANSFER:
			{
				Account_ID_t account_id = bytes.pop<Account_ID_t>();
				Hash_t cookie = bytes.pop<Hash_t>();

				if(sessions.Size() >= MAX_CLIENTS)
					return ByteQueue::Create<byte>(ERR_OUT_OF_CAPACITY);
				
				if(!sessions.Insert(account_id, cookie, nullptr))
					return ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_MATCH);
			}
			return ByteQueue::Create<byte>(SUCCESS);
	}

	return ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
}

void MyMatch::MatchMake()
{
	Thread::SetThreadName("MatchMaker");

	while(isRunning)
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
			std::shared_ptr<Notifier> lpnotifier;
			std::shared_ptr<Notifier> rpnotifier;
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
				{
					//모든 Battle 서버에 허용량 확인하기.
					std::mutex req_mtx;
					std::map<std::shared_ptr<MyConnector>, int> capacities;

					connectee.Request("battle", ByteQueue::Create<byte>(INQ_AVAILABLE), [&](std::shared_ptr<MyConnector> connector, ByteQueue answer)
					{
						byte header = answer.pop<byte>();
						switch(header)
						{
							case SUCCESS:
								{
									int capacity = answer.pop<int>();
									std::unique_lock<std::mutex> lk(req_mtx);
									capacities.insert({connector, capacity});
								}
								break;
							case ERR_OUT_OF_CAPACITY:
								break;
						}
					});

					//가장 capacity가 많은 Connector를 찾아 request 하기.
					auto maxiter = std::max_element(capacities.begin(), capacities.end(), [](std::pair<std::shared_ptr<MyConnector>, int> const &lhs, std::pair<std::shared_ptr<MyConnector>, int> const &rhs){ return lhs.second < rhs.second; });
					if(maxiter == capacities.end() || maxiter->second <= 0)
					{
						Logger::log("Battle Server Cannot Accept Battle : " + ErrorCode(ERR_OUT_OF_CAPACITY).message_code(), Logger::LogType::error);
						break;
					}

					//battle서버에 battle 등록.
					ByteQueue req = ByteQueue::Create<byte>(INQ_MATCH_TRANSFER);
					req.push<Account_ID_t>(lpid);
					req.push<Hash_t>(sessions.FindLKey(lpid)->first);
					req.push<Account_ID_t>(rpid);
					req.push<Hash_t>(sessions.FindLKey(rpid)->first);

					auto req_result = maxiter->first->Request(req);
					if(!req_result)
					{
						Logger::log("Battle Server Cannot Accept Battle : " + req_result.error().message_code(), Logger::LogType::error);
						break;
					}
					ByteQueue ans = *req_result;
					byte result = ans.pop<byte>();
					if(result == SUCCESS)
					{
						int battle_server = ans.pop<Seed_t>();
						Logger::log("Match made : " + std::to_string(lpid) + " vs " + std::to_string(rpid) + " onto " + std::to_string(battle_server), Logger::LogType::info);
						lpnotifier->push(std::make_shared<MyMatchMakerMessage>(battle_server));
						rpnotifier->push(std::make_shared<MyMatchMakerMessage>(battle_server));
						matchmaker.PopMatch();

						sessions.EraseLKey(lpid);
						sessions.EraseLKey(rpid);
					}
					else	//battle서버에서 매치 전달에 실패한 경우
					{
						Logger::log("Battle Server Cannot Accept Battle : " + ErrorCode(result).message_code(), Logger::LogType::error);
						break;
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