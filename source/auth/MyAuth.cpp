#include "MyAuth.hpp"

MyAuth::MyAuth() : 
	connector("auth", 1, this), keyword_match("match"),
	MyServer(ConfigParser::GetInt("Auth_ClientPort_Web", 54322), ConfigParser::GetInt("Auth_ClientPort_TCP", 54422))
{
}

MyAuth::~MyAuth()
{
}

void MyAuth::Open()
{
	//connectee.Accept("battle", std::bind(MyAuth::AuthGenie, this, std::placeholders::_1));	//battle 서버가 auth서버로 질의를 할 일은 없다.
	MyPostgres::Open();
	connector.Connect(ConfigParser::GetString("Match_Addr"), ConfigParser::GetInt("Match_Port"), keyword_match,  std::bind(&MyAuth::AuthInquiry, this, std::placeholders::_1));
	Logger::log("Auth Server Start", Logger::LogType::info);
}

void MyAuth::Close()
{
	connector.Close();
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
	byte header = request.pop<byte>();
	switch(header)
	{
		default:
			answer = ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
			break;
	}
	return answer;
}