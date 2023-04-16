#include "MyAuth.hpp"

MyAuth::MyAuth() : 
	connector_match(this, ConfigParser::GetString("Match1_Addr"), ConfigParser::GetInt("Match1_Port"), "auth"),
	MyServer(ConfigParser::GetInt("Auth_ClientPort_Web", 54321), ConfigParser::GetInt("Auth_ClientPort_TCP", 54322))
{
}

MyAuth::~MyAuth()
{
}

void MyAuth::Open()
{
	//connectee.Accept("battle", std::bind(MyAuth::AuthGenie, this, std::placeholders::_1));	//battle 서버가 auth서버로 질의를 할 일은 없다.
	MyPostgres::Open();
	connector_match.Connect();
	Logger::log("Auth Server Start", Logger::LogType::info);
}

void MyAuth::Close()
{
	connector_match.Disconnect();
	MyPostgres::Close();
	Logger::log("Auth Server Stop", Logger::LogType::info);
}

void MyAuth::ClientProcess(std::shared_ptr<MyClientSocket> client)
{
	bool exit_client = false;

	Logger::log("Client " + client->ToString() + " Entered", Logger::LogType::auth);

	StackErrorCode sec = client->KeyExchange();
	if(!sec)
		Logger::log("Client " + client->ToString() + " Failed to KeyExchange", Logger::LogType::auth);
	while(isRunning && !exit_client && sec)
	{
		auto recv = client->Recv();
		if(!recv)
		{
			sec = StackErrorCode{recv.error(), __STACKINFO__};
			break;
		}

		try
		{
			byte header = recv->pop<byte>();

			std::string email;
			Pwd_Hash_t pwd_hash;
			Account_ID_t account_id = 0;
			Seed_t match_id = 1;

			switch(header)
			{
				case REQ_REGISTER:
				case REQ_LOGIN:
				//case REQ_CHPWD:
					pwd_hash = recv->pop<Pwd_Hash_t>();
					email = recv->popstr();
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
					Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
					sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT)), __STACKINFO__};
					continue;
				}
				else
				{
					Logger::log("Client " + client->ToString() + " Success to Login \'" + std::to_string(account_id) + "\'", Logger::LogType::auth);
				}
				db.commit();
			}
			catch(const pqxx::unique_violation &e)
			{
				sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT)), __STACKINFO__};
				continue;
			}
			catch(const pqxx::plpgsql_no_data_found &e)
			{
				sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT)), __STACKINFO__};
				continue;
			}
			catch(const pqxx::unexpected_rows &e)
			{
				sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT)), __STACKINFO__};
				continue;
			}
			catch(const pqxx::pqxx_exception &e)
			{
				Logger::log("DB Error : " + std::string(e.base().what()), Logger::LogType::error);
				sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED)), __STACKINFO__};
				continue;
			}

			//match/battle 서버에 cookie 확인. battle 서버는 match서버가 확인해 줄 것임.
			{
				ByteQueue query = ByteQueue::Create<byte>(INQ_ACCOUNT_CHECK);
				match_id = 1;	//TODO : 추후, connector_match.Request()가 ERR_OUT_OF_CAPACITY를 반환했을 때, 다음 match서버로 넘기는 기능 추가 필요.
				ByteQueue account_byte = ByteQueue::Create<Account_ID_t>(account_id);
				ByteQueue match_server_byte = ByteQueue::Create<Seed_t>(match_id);
				query += account_byte;
				ByteQueue answer = connector_match.Request(query);
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
								auto cookieres = connector_match.Request(cookiereq);
								cookieheader = cookieres.pop<byte>();
							}
							switch(cookieheader)
							{
								case SUCCESS:
									sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(SUCCESS) + cookie + match_server_byte), __STACKINFO__};
									exit_client = true;
									break;
								default:	//TODO : ERR_OUT_OF_CAPACITY가 오면 타 Match 서버로 연결해야 함.
									sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(cookieheader)), __STACKINFO__};
									exit_client = true;
									break;
							}
						}
						break;
					case ERR_EXIST_ACCOUNT_MATCH:	//쿠키와 몇번 매치서버인지(unsigned int)는 뒤에 붙어있다.
					case ERR_EXIST_ACCOUNT_BATTLE:	//쿠키와 몇번 배틀서버인지(unsigned int)는 뒤에 붙어있다.
					case ERR_OUT_OF_CAPACITY:		//서버 접속 불가. 단일 byte.
						sec = StackErrorCode{client->Send(answer), __STACKINFO__};
						exit_client = true;
						break;
					default:
						throw StackTraceExcept("Unknown Header From Match : " + std::to_string(query_header), __STACKINFO__);
				}
			}

			break;
		}
		catch(const std::exception& e)
		{
			sec = StackErrorCode{client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION)), __STACKINFO__};
			Logger::log("Client " + client->ToString() + " : " + std::string(e.what()), Logger::LogType::error);
		}
	}

	client->Close();
	Logger::log("Client " + client->ToString() + " Exit", Logger::LogType::auth);

	if(!client->isNormalClose(sec))
		throw ErrorCodeExcept(sec, __STACKINFO__);
}