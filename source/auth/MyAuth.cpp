#include "MyAuth.hpp"

MyAuth::MyAuth() : 
	connector_match(this, MyConfigParser::GetString("Match1_Addr"), MyConfigParser::GetInt("Match1_Port"), "auth"),
	MyServer(MyConfigParser::GetInt("Auth_ClientPort_Web", 54321), MyConfigParser::GetInt("Auth_ClientPort_TCP", 54322))
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
	MyLogger::log("Auth Server Start", MyLogger::LogType::info);
}

void MyAuth::Close()
{
	connector_match.Disconnect();
	MyPostgres::Close();
	MyLogger::log("Auth Server Stop", MyLogger::LogType::info);
}

void MyAuth::ClientProcess(std::shared_ptr<MyClientSocket> client)
{
	bool exit_client = false;
	while(isRunning || !exit_client)
	{
		auto recv = client->Recv();
		if(!recv)
		{
			if(recv.error() >= 0)	//-1이면 close()로 인한 연결 종료.
				throw MyExcepts("Unknown Client Recv Failed : " + std::to_string(recv.error()), __STACKINFO__);
			break;
		}

		try
		{
			byte header = recv->pop<byte>();
			switch(header)
			{
				case REQ_REGISTER:
				case REQ_LOGIN:
				case REQ_CHPWD:
					break;
				default:
					throw MyExcepts("Protocol Violation(Unknown Header \'" + std::to_string(header) + "\')", __STACKINFO__);
			}
			Pwd_Hash_t pwd_hash;
			Pwd_Hash_t new_pwd_hash;
			std::string email;

			pwd_hash = recv->pop<Pwd_Hash_t>();
			if(header == REQ_CHPWD)
				new_pwd_hash = recv->pop<Pwd_Hash_t>();
			email = recv->popstr();
			
			Account_ID_t account_id = 0;
			try
			{
				if(header == REQ_REGISTER)
				{
					dbsystem reg_db;
					reg_db.exec("INSERT INTO userlist (email, pwd_hash) VALUES (\'" + reg_db.quote(email) + "\', \'" + reg_db.quote_raw(pwd_hash.data(), sizeof(Pwd_Hash_t)) + "\')");
					reg_db.commit();
				}

				dbsystem db;
				std::string search_query = "SELECT id FROM userlist WHERE email = \'" + db.quote(email) + "\' AND pwd_hash = \'" + db.quote_raw(pwd_hash.data(), sizeof(Pwd_Hash_t)) + "\'";
				pqxx::row result = db.exec1(search_query);
				account_id = result[0].as<Account_ID_t>();
				if(header == REQ_CHPWD)
				{
					db.exec("UPDATE userlist SET pwd_hash = \'" + db.quote_raw(new_pwd_hash.data(), sizeof(Pwd_Hash_t)) + ", access_time = now() WHERE id = \'" + std::to_string(account_id) + "\'");
					db.commit();
				}
			}
			catch(const pqxx::plpgsql_no_data_found &e)
			{
				client->Send(MyBytes::Create<byte>(ERR_NO_MATCH_ACCOUNT));
				continue;
			}
			catch(const pqxx::unique_violation &e)
			{
				client->Send(MyBytes::Create<byte>(ERR_EXIST_ACCOUNT));
				continue;
			}

			//match/battle 서버에 cookie 확인. battle 서버는 match서버가 확인해 줄 것임.
			{
				MyBytes query = MyBytes::Create<byte>(INQ_ACCOUNT_CHECK);
				query.push<Account_ID_t>(account_id);
				MyBytes answer = connector_match.Request(query);
				byte query_header = answer.pop<byte>();
				switch(query_header)
				{
					case ERR_NO_MATCH_ACCOUNT:
						{
							byte cookieheader = !SUCCESS;	//success만 아니면 된다.
							while(cookieheader != SUCCESS)	//COOKIE_TRANSFER가 성공할 때 까지(즉, 중복 cookie가 없을때 까지) 반복
							{
								MyBytes seed = MyBytes::Create<Account_ID_t>(account_id);
								seed.push<std::chrono::time_point<std::chrono::system_clock>>(std::chrono::system_clock::now());
								MyBytes cookie = MyBytes::Create<Hash_t>(MyCommon::Hash::sha256(seed));
								MyBytes cookiereq = MyBytes::Create<byte>(INQ_COOKIE_TRANSFER) + cookie;
								auto cookieres = connector_match.Request(cookiereq);
								byte cookieheader = cookieres.pop<byte>();
								if(cookieheader == SUCCESS)
								{
									client->Send(MyBytes::Create<byte>(SUCCESS) + cookie);
									exit_client = true;
								}
							}
						}
						break;
					case ERR_EXIST_ACCOUNT_MATCH:	//쿠키와 몇번 매치서버인지(unsigned int)는 뒤에 붙어있다.
					case ERR_EXIST_ACCOUNT_BATTLE:	//쿠키와 몇번 배틀서버인지(unsigned int)는 뒤에 붙어있다.
					case ERR_OUT_OF_CAPACITY:		//서버 접속 불가. 단일 byte.
						client->Send(answer);
						exit_client = true;
						break;
					default:
						throw MyExcepts("Unknown Header : " + std::to_string(query_header), __STACKINFO__);
				}
			}

			break;
		}
		catch(const std::exception& e)
		{
			client->Send(MyBytes::Create<byte>(ERR_PROTOCOL_VIOLATION));
			MyLogger::raise();
		}
	}

	client->Close();
}