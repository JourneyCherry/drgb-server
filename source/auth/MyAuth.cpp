#include "MyAuth.hpp"

MyAuth::MyAuth() : 
	redis(SESSION_TIMEOUT),
	connector("auth", 1, this), keyword_match("match"),
	connectee("auth", ConfigParser::GetInt("Auth_Port", 52431), 1, this),
	MyServer(ConfigParser::GetInt("Auth_ClientPort_Web", 54322), ConfigParser::GetInt("Auth_ClientPort_TCP", 54422))
{
}

MyAuth::~MyAuth()
{
}

void MyAuth::Open()
{
	MyPostgres::Open();
	redis.Connect(ConfigParser::GetString("SessionAddr"), ConfigParser::GetInt("SessionPort", 6379));
	connectee.Accept("drgbmgr", std::bind(&MyAuth::AuthInquiry, this, std::placeholders::_1));
	connector.Connect(ConfigParser::GetString("Match_Addr"), ConfigParser::GetInt("Match_Port"), keyword_match,  std::bind(&MyAuth::AuthInquiry, this, std::placeholders::_1));
	Logger::log("Auth Server Start", Logger::LogType::info);
}

void MyAuth::Close()
{
	connectee.Close();
	connector.Close();
	redis.Close();
	MyPostgres::Close();
	Logger::log("Auth Server Stop", Logger::LogType::info);
}

void MyAuth::AcceptProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec)
		return;
		
	client->KeyExchange(std::bind(&MyAuth::EnterProcess, this, std::placeholders::_1, std::placeholders::_2));
}

void MyAuth::EnterProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec.isSuccessed() || !client->is_open())
	{
		client->Close();
		Logger::log("Client " + client->ToString() + " Failed to Key Exchange", Logger::LogType::auth);
	}
	else
	{
		Logger::log("Client " + client->ToString() + " Entered", Logger::LogType::auth);
		client->StartRecv(std::bind(&MyAuth::ClientProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

void MyAuth::ClientProcess(std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
{
	if(!recv_ec)
	{
		if(client->isNormalClose(recv_ec))
			Logger::log("Client " + client->ToString() + " Exited", Logger::LogType::auth);
		else
			Logger::log("Client " + client->ToString() + " Exited with " + recv_ec.message_code(), Logger::LogType::auth);
		client->Close();
		return;
	}

	//Authenticate
	try
	{
		byte header = packet.pop<byte>();

		std::string email;
		Pwd_Hash_t pwd_hash;
		Account_ID_t account_id = 0;

		switch(header)
		{
			case REQ_REGISTER:
			case REQ_LOGIN:
			//case REQ_CHPWD:
				pwd_hash = packet.pop<Pwd_Hash_t>();
				email = packet.popstr();
				break;
			default:
				throw StackTraceExcept("Unknown Header \'" + std::to_string(header) + "\'", __STACKINFO__);
		}

		try
		{
			account_id = 0;
			dbsystem db;
			if(header == REQ_REGISTER)
				account_id = db.RegisterAccount(email, pwd_hash);
			else if(header == REQ_LOGIN)
				account_id = db.FindAccount(email, pwd_hash);
			if(account_id <= 0)
			{
				account_id = 0;
				Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\' for Password Mismatch", Logger::LogType::auth);
				client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
				return;
			}
			else
			{
				Logger::log("Client " + client->ToString() + " Success to Login \'" + std::to_string(account_id) + "\'", Logger::LogType::auth);
			}
			db.commit();
		}
		catch(const pqxx::unique_violation &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT));
			return;
		}
		catch(const pqxx::plpgsql_no_data_found &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
			return;
		}
		catch(const pqxx::unexpected_rows &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
			return;
		}
		catch(const pqxx::pqxx_exception &e)
		{
			Logger::log("DB Error : " + std::string(e.base().what()), Logger::LogType::error);
			client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
			return;
		}

		//session check
		{
			auto info = redis.GetInfoFromID(account_id);
			if(!info)
			{
				if(info.error().code() == ERR_CONNECTION_CLOSED)
				{
					client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
					return;
				}
				ByteQueue seed = ByteQueue::Create<Account_ID_t>(account_id) + ByteQueue::Create<std::chrono::time_point<std::chrono::system_clock>>(std::chrono::system_clock::now());
				Hash_t cookie = Hasher::sha256(seed);

				//중복 쿠키 확인후, 존재하면 다시 해시하기.
				while(redis.GetInfoFromCookie(cookie).isSuccessed())
					cookie = Hasher::sha256(cookie.data(), cookie.size());

				auto result = redis.SetInfo(account_id, cookie, keyword_match);
				if(!result)
					client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
				else
				{
					client->Send(ByteQueue::Create<byte>(SUCCESS) + ByteQueue::Create<Hash_t>(cookie));
					client->Close();
				}
			}
			else
			{
				ByteQueue cookie = ByteQueue::Create<Hash_t>(info->first);
				std::string serverName = info->second;
				//TODO : match에 질의해서 해당 keyword의 서버가 연결되어 있는지, 주소는 어디인지 받아오기.(match 자기 자신 포함)
				if(serverName == keyword_match)
					client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_MATCH) + cookie);
				else
					client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_BATTLE) + ByteQueue::Create<Seed_t>(1));
				client->Close();
			}
		}

		/*
		//match/battle 서버에 cookie 확인. battle 서버는 match서버가 확인해 줄 것임.
		{
			ByteQueue query = ByteQueue::Create<byte>(INQ_ACCOUNT_CHECK);
			//Seed_t match_id = 1;	//TODO : 추후, match 서버가 distributed 될 때, Load Balancing 하도록 변경 필요.
			ByteQueue account_byte = ByteQueue::Create<Account_ID_t>(account_id);
			query += account_byte;
			auto req_result = connector.Request(keyword_match, query);
			if(!req_result)
			{
				client->Send(ByteQueue::Create<byte>(ERR_OUT_OF_CAPACITY));
				client->Close();
				return;
			}
			ByteQueue answer = *req_result;
			byte query_header = answer.pop<byte>();
			switch(query_header)
			{
				case ERR_NO_MATCH_ACCOUNT:
					{
						byte cookieheader = ERR_EXIST_ACCOUNT_MATCH;
						ByteQueue cookie;
						while(cookieheader == ERR_EXIST_ACCOUNT_MATCH)	//COOKIE_TRANSFER가 성공할 때 까지(즉, 중복 cookie가 없을때 까지) 반복
						{
							ByteQueue seed = account_byte + ByteQueue::Create<std::chrono::time_point<std::chrono::system_clock>>(std::chrono::system_clock::now());
							cookie = ByteQueue::Create<Hash_t>(Hasher::sha256(seed));
							ByteQueue cookiereq = ByteQueue::Create<byte>(INQ_COOKIE_TRANSFER) + account_byte + cookie;
							auto cookieres = connector.Request(keyword_match, cookiereq);
							if(!cookieres)
								continue;
							cookieheader = cookieres->pop<byte>();
						}
						switch(cookieheader)
						{
							case SUCCESS:
								client->Send(ByteQueue::Create<byte>(SUCCESS) + cookie);
								client->Close();
								break;
							default:
								client->Send(ByteQueue::Create<byte>(cookieheader));
								client->Close();
								break;
						}
					}
					break;
				case ERR_EXIST_ACCOUNT_MATCH:	//쿠키는 뒤에 붙어있다.
				case ERR_EXIST_ACCOUNT_BATTLE:	//쿠키와 몇번 배틀서버인지(Seed_t)는 뒤에 붙어있다.
				case ERR_OUT_OF_CAPACITY:		//서버 접속 불가. 단일 byte.
					client->Send(answer);
					client->Close();
					break;
				default:
					throw StackTraceExcept("Unknown Header From Match : " + std::to_string(query_header), __STACKINFO__);
			}
		}
		*/
	}
	catch(const std::exception& e)
	{
		client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
		Logger::log("Client " + client->ToString() + " : " + std::string(e.what()), Logger::LogType::error);
	}
}

ByteQueue MyAuth::AuthInquiry(ByteQueue request)
{
	ByteQueue answer;
	try
	{
		byte header = request.pop<byte>();
		switch(header)
		{
			case INQ_CLIENTUSAGE:
				answer = ByteQueue::Create<byte>(SUCCESS);
				answer.push<size_t>(tcp_server.GetConnected());
				answer.push<size_t>(web_server.GetConnected());
				break;
			case INQ_CONNUSAGE:
				{
					answer = ByteQueue::Create<byte>(SUCCESS);

					auto teeusage = connectee.GetUsage();
					answer.push<size_t>(teeusage.size());	//connectee 목록
					for(auto &tee : teeusage)
					{
						answer.push<size_t>(tee.first.size());
						answer.push<std::string>(tee.first);
						answer.push<size_t>(tee.second);
					}
					auto torusage = connector.GetUsage();
					answer.push<size_t>(torusage.size());	//connector 목록
					for(auto &tor : torusage)
					{
						answer.push<size_t>(tor.first.size());
						answer.push<std::string>(tor.first);
						answer.push<int>(tor.second);
					}
				}
				break;
			default:
				answer = ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
				break;
		}
	}
	catch(...)
	{
		answer = ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
	}
	return answer;
}