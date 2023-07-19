#include "MyMatch.hpp"

const std::string MyMatch::self_keyword = "match";

MyMatch::MyMatch() : 
	rematch_delay(ConfigParser::GetInt("Match_Delay", 1000)),
	matchmaker(ConfigParser::GetDouble("Match_Interpolate", 0.1)),
	MyServer(ConfigParser::GetInt("Match_ClientPort", 54323))
{
	ServiceBuilder.RegisterService(&MatchService);
}

MyMatch::~MyMatch()
{
}

void MyMatch::Open()
{
	redis.Connect(ConfigParser::GetString("SessionAddr"), ConfigParser::GetInt("SessionPort", 6379));
	t_matchmaker = std::thread(std::bind(&MyMatch::MatchMake, this));
	Logger::log("Match Server Start", Logger::LogType::info);
}

void MyMatch::Close()
{
	redis.Close();
	matchmaker_cv.notify_all();	//isRunning은 MyServer::Stop()에서 false로 변경되며, MyMatch::Close()보다 먼저 수행됨.
	if(t_matchmaker.joinable())
		t_matchmaker.join();
	Logger::log("Match Server Stop", Logger::LogType::info);
}

void MyMatch::AcceptProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec)
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : " + ec.message_code(), Logger::LogType::auth);
		client->Close();
		return;
	}

	client->StartRecv(std::bind(&MyMatch::AuthenticateProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	client->SetTimeout(TIME_AUTHENTICATE, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
}

void MyMatch::AuthenticateProcess(std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
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
	if(info->second != self_keyword)	//Session은 있지만 대상이 Match 서버가 아닌 경우
	{
		Logger::log("Client " + client->ToString() + " Failed to Authenticate : Session Mismatched", Logger::LogType::auth);
		client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		client->Close();
		return;
	}

	//Local Server에서 Session 찾기
	auto session = this->sessions.FindRKey(cookie);
	if(session)	//기존에 사용자가 접속한 경우
	{
		auto origin = session->second;
		if(!origin.expired())	//Dup-Access Process
		{
			this->matchmaker.Exit(account_id);
			auto origin_ptr = origin.lock();
			Logger::log("Account " + std::to_string(account_id) + " has dup access : " + origin_ptr->ToString() + " -> " + client->ToString(), Logger::LogType::auth);
			origin_ptr->Send(ByteQueue::Create<byte>(ERR_DUPLICATED_ACCESS));
			origin_ptr->Close();
		}
		this->sessions.InsertLKeyValue(account_id, client->weak_from_this());
	}
	else	//새 접속인 경우
		this->sessions.Insert(account_id, cookie, client->weak_from_this());

	//사용자에게 정보 전달.
	try
	{
		auto db = dbpool.GetConnection();
		if(!db)
			throw StackTraceExcept("DB Connection Failed", __STACKINFO__);
		ByteQueue infopacket = ByteQueue::Create<byte>(GAME_PLAYER_INFO);
		auto [nickname, win, draw, loose] = db->GetInfo(account_id);
		infopacket.push<Achievement_ID_t>(nickname);	//nickname
		infopacket.push<int>(win);	//win
		infopacket.push<int>(draw);	//draw
		infopacket.push<int>(loose);	//loose

		auto result = db->GetAllAchieve(account_id);
		for(auto pair : result)
		{
			infopacket.push<Achievement_ID_t>(pair.first);	//achieve id
			infopacket.push<int>(pair.second);	//achieve count
		}
		client->Send(infopacket);
	}
	catch(const pqxx::pqxx_exception & e)
	{
		client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
		client->Close();
		throw StackTraceExcept("DB Failed : " + std::string(e.base().what()), __STACKINFO__);
	}
	catch(const std::exception &e)
	{
		client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
		client->Close();
		throw StackTraceExcept("DB Failed : " + std::string(e.what()), __STACKINFO__);
	}

	Logger::log("Account " + std::to_string(account_id) + " logged in from " + client->ToString(), Logger::LogType::auth);
	ClientProcess(client, account_id);
	SessionProcess(client, account_id, cookie);
}

void MyMatch::ClientProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id)
{
	target_client->StartRecv([this, account_id](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		if(!recv_ec)
		{
			if(client->isNormalClose(recv_ec))	//TODO : 아무것도 안하고 나갔을 경우, 세션이 제대로 빠지지 않는다. 확인 필요.
				Logger::log("Account " + std::to_string(account_id) + " logged out", Logger::LogType::auth);
			else
				Logger::log("Account " + std::to_string(account_id) + " logged out with " + recv_ec.message_code(), Logger::LogType::auth);
			matchmaker.Exit(account_id);
			auto session = sessions.FindLKey(account_id);
			if(!session)
				return;
			if(session->second.lock() == client)	//dup-accessed when it is false
				sessions.EraseLKey(account_id);
			client->Close();
			return;
		}

		try
		{
			byte header = packet.pop<byte>();
			switch(header)
			{
				case ANS_HEARTBEAT:
					break;
				case REQ_CHPWD:
					try
					{
						Pwd_Hash_t old_pwd = packet.pop<Pwd_Hash_t>();
						Pwd_Hash_t new_pwd = packet.pop<Pwd_Hash_t>();
						auto db = dbpool.GetConnection();
						if(!db)
							throw pqxx::broken_connection();
						if(db->ChangePwd(account_id, old_pwd, new_pwd))
						{
							db->commit();
							client->Send(ByteQueue::Create<byte>(SUCCESS));
						}
						else
						{
							client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
						}
					}
					catch(const pqxx::pqxx_exception & e)
					{
						client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
						Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::debug);
					}
					break;
				case REQ_STARTMATCH:	//매치메이커 큐에 넣기.
					{
						auto db = dbpool.GetConnection();
						if(!db)
							throw pqxx::broken_connection();
						auto [_, win, draw, loose] = db->GetInfo(account_id);
						matchmaker.Enter(account_id, win, draw, loose);
						matchmaker_cv.notify_one();
					}
					break;
				case REQ_PAUSEMATCH:	//매치메이커 큐에서 빼기.
					matchmaker.Exit(account_id);
					break;
				case REQ_CHNAME:
					{
						Achievement_ID_t nameindex = packet.pop<Achievement_ID_t>();
						auto db = dbpool.GetConnection();
						if(!db)
							throw pqxx::broken_connection();
						db->SetNickName(account_id, nameindex);	//성공하든, 실패하든 항상 정보를 다시 던져준다.

						ByteQueue infopacket = ByteQueue::Create<byte>(GAME_PLAYER_INFO);
						auto [nickname, win, draw, loose] = db->GetInfo(account_id);
						infopacket.push<Achievement_ID_t>(nickname);
						infopacket.push<int>(win);
						infopacket.push<int>(draw);	
						infopacket.push<int>(loose);

						db->commit();
						client->Send(infopacket);
					}
					break;
				default:
					client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
					break;
			}
		}
		catch(const pqxx::pqxx_exception & e)
		{
			Logger::log("DB Failed : " + std::string(e.base().what()), Logger::LogType::debug);
			client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
			client->Close();
		}
		catch(...)
		{
			client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
			ThrowThreadException(std::current_exception());
		}
		ClientProcess(client, account_id);
	});
}

void MyMatch::SessionProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, Hash_t cookie)
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

void MyMatch::MatchMake()
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "MatchMaker");
#endif

	while(isRunning)
	{
		try
		{
			std::this_thread::sleep_for(rematch_delay);
			std::unique_lock lk(matchmaker_mtx);
			matchmaker_cv.wait(lk, [this](){ return (matchmaker.Size() >= 2 || !isRunning); });
			lk.unlock();
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
				std::shared_ptr<MyClientSocket> lpsocket;
				std::shared_ptr<MyClientSocket> rpsocket;
				if(lpresult)
				{
					lpcookie = lpresult->first;
					lpsocket = lpresult->second.lock();
					if(!lpsocket->Send(ByteQueue::Create<byte>(ANS_HEARTBEAT)))
					{
						lpsocket->Close();
						lpsocket = nullptr;
					}
					else if(!redis.RefreshInfo(lpid, lpcookie, self_keyword))
					{
						lpsocket->Close();
						lpsocket = nullptr;
					}
				}
				if(rpresult)
				{
					rpcookie = rpresult->first;
					rpsocket = rpresult->second.lock();
					if(!rpsocket->Send(ByteQueue::Create<byte>(ANS_HEARTBEAT)))
					{
						rpsocket->Close();
						rpsocket = nullptr;
					}
					else if(!redis.RefreshInfo(rpid, rpcookie, self_keyword))
					{
						rpsocket->Close();
						rpsocket = nullptr;
					}
				}
				
				if(lpsocket && rpsocket)	//둘다 정상접속 상태이면
				{
					try
					{
						//모든 Battle 서버에 허용량 확인하고 가장 capacity가 많은 서버를 찾아 Match 전달하기.
						Seed_t battle_server = MatchService.TransferMatch(lpid, rpid);
						if(battle_server <= 0)
							throw ErrorCodeExcept(ERR_OUT_OF_CAPACITY, __STACKINFO__);

						std::string battle_server_name = "battle" + std::to_string(battle_server);

						//Session 서버 등록.
						redis.SetInfo(lpid, lpcookie, battle_server_name);
						redis.SetInfo(rpid, rpcookie, battle_server_name);

						Logger::log("Match made : " + std::to_string(lpid) + " vs " + std::to_string(rpid) + " onto " + std::to_string(battle_server), Logger::LogType::info);
						ByteQueue msg_to_client = ByteQueue::Create<byte>(ANS_MATCHMADE);
						msg_to_client.push<int>(battle_server);
						lpsocket->Send(msg_to_client);
						rpsocket->Send(msg_to_client);
						matchmaker.PopMatch();

						sessions.EraseLKey(lpid);
						sessions.EraseLKey(rpid);

						lpsocket->Close();
						rpsocket->Close();
					}
					catch(const std::exception &e)
					{
						Logger::log("Battle Server Cannot Accept : " + std::string(e.what()), Logger::LogType::error);
						break;
					}
				}
				else	//매칭된 둘 중에 한명이라도 이미 접속을 종료했다면 남은 한명을 다시 큐에 집어넣는다.
				{
					if(lpsocket) matchmaker.Enter(lp);
					else		sessions.EraseLKey(lpid);	//연결된 소켓이 없으면 세션을 비운다.
					if(rpsocket) matchmaker.Enter(rp);
					else		sessions.EraseLKey(rpid);	//연결된 소켓이 없으면 세션을 비운다.
					matchmaker.PopMatch();
				}
			}
		}
		catch(...)
		{
			ThrowThreadException(std::current_exception());
		}
	}
}

bool MyMatch::CheckAccount(Account_ID_t id)
{
	return sessions.FindLKey(id).isSuccessed();
}

std::map<std::string, size_t> MyMatch::GetConnectUsage()
{
	auto connected = MyServer::GetConnectUsage();
	auto streams = MatchService.GetStreams();
	for(auto &seed : streams)
		connected.insert({"battle" + std::to_string(seed), 1});
	return connected;
}