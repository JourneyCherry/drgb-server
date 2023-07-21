#include "MyAuth.hpp"

MyAuth::MyAuth() : 
	MyServer(ConfigParser::GetInt("Client_Port", 54321))
{
}

MyAuth::~MyAuth()
{
}

void MyAuth::Open()
{
	Logger::log("Auth Server Start", Logger::LogType::info);
}

void MyAuth::Close()
{
	Logger::log("Auth Server Stop", Logger::LogType::info);
}

void MyAuth::AcceptProcess(std::shared_ptr<MyClientSocket> client, ErrorCode ec)
{
	if(!ec.isSuccessed() || !client->is_open())
	{
		client->Close();
		Logger::log("Client " + client->ToString() + " Failed to Key Exchange", Logger::LogType::auth);
	}
	else
	{
		Logger::log("Client " + client->ToString() + " Entered", Logger::LogType::auth);
		ClientProcess(client);
		SessionProcess(client);
	}
}

void MyAuth::ClientProcess(std::shared_ptr<MyClientSocket> target_client)
{
	target_client->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
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

		std::string email;
		Pwd_Hash_t pwd_hash;
		Account_ID_t account_id = 0;

		//Authenticate
		try
		{
			byte header = packet.pop<byte>();

			switch(header)
			{
				case REQ_REGISTER:
				case REQ_LOGIN:
					pwd_hash = packet.pop<Pwd_Hash_t>();
					email = packet.popstr();
					break;
				default:
					throw StackTraceExcept("Unknown Header \'" + std::to_string(header) + "\'", __STACKINFO__);
			}

			//Chceck Account from DB
			{
				auto db = dbpool.GetConnection();
				if(header == REQ_REGISTER)
					account_id = db->RegisterAccount(email, pwd_hash);
				else if(header == REQ_LOGIN)
					account_id = db->FindAccount(email, pwd_hash);
				if(account_id <= 0)
				{
					account_id = 0;
					Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\' for Password Mismatch", Logger::LogType::auth);
					client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
					db->abort();
				}
				else
				{
					Logger::log("Client " + client->ToString() + " Success to Login \'" + std::to_string(account_id) + "\'", Logger::LogType::auth);
					db->commit();
				}
			}

			//session check
			{
				auto info = redis.GetInfoFromID(account_id);
				if(!info)
				{
					if(info.error().code() == ERR_CONNECTION_CLOSED)
					{
						client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
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
					if(serverName == keyword_match)	//Session이 Match 서버에 연결된 경우
						client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_MATCH) + cookie);
					else
					{
						std::regex reg("\\d+$");	//여기서 battle까지 탐색하면 result[0]에 숫자만이 아니라 battle문자도 같이 들어간다.
						std::smatch result;
						Seed_t server_id = -1;
						if(std::regex_search(serverName, result, reg))
						{
							try
							{
								if(result.prefix() != "battle")
									server_id = -1;
								else
									server_id = std::stoi(result[0]);
							}
							catch(...)
							{
								server_id = -1;
							}
						}
						if(server_id > 0)
							client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT_BATTLE) + cookie + ByteQueue::Create<Seed_t>(server_id));
						else		//Cookie는 있는데 Session이 알 수 없는 서버에 연결된 경우
							client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
					}
					client->Close();
				}
			}
		}
		catch(const pqxx::unique_violation &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_EXIST_ACCOUNT));
		}
		catch(const pqxx::plpgsql_no_data_found &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		}
		catch(const pqxx::unexpected_rows &e)
		{
			Logger::log("Client " + client->ToString() + " Failed to login \'" + email + "\'", Logger::LogType::auth);
			client->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_ACCOUNT));
		}
		catch(const pqxx::pqxx_exception &e)
		{
			Logger::log("DB Error : " + std::string(e.base().what()), Logger::LogType::error);
			client->Send(ByteQueue::Create<byte>(ERR_DB_FAILED));
		}
		catch(const std::exception& e)
		{
			client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
			Logger::log("Client " + client->ToString() + " : " + std::string(e.what()), Logger::LogType::error);
		}

		ClientProcess(client);
	});
}

void MyAuth::SessionProcess(std::shared_ptr<MyClientSocket> target_client)
{
	target_client->SetTimeout(SESSION_TIMEOUT / 2, [this](std::shared_ptr<MyClientSocket> client)
	{
		if(!client->is_open())
			return;
		
		if(!client->Send(ByteQueue::Create<byte>(ANS_HEARTBEAT)))
		{
			client->Close();
			return;
		}
		SessionProcess(client);
	});
}