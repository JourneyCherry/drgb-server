#include "TestClient.hpp"

std::vector<std::pair<std::string, int>> TestClient::addrs;
ThreadExceptHandler *TestClient::parent = nullptr;
boost::asio::io_context *TestClient::pioc = nullptr;
const int TestClient::LoginTimeout = 5000;
std::random_device TestClient::rd;
std::mt19937_64 TestClient::gen(rd());
std::uniform_int_distribution<int> TestClient::dis(0, 4);
std::uniform_int_distribution<int> TestClient::millidis(100, 1000);
bool TestClient::doNotRestart = false;
bool TestClient::isRunning = true;

void TestClient::SetRestart(const bool& notRestart)
{
	doNotRestart = notRestart;
}

void TestClient::AddAddr(std::string addr, int port)
{
	addrs.push_back(std::make_pair(addr, port));
}

void TestClient::SetInfo(ThreadExceptHandler *_parent, boost::asio::io_context* _pioc)
{
	parent = _parent;
	pioc = _pioc;
}

int TestClient::GetRandomAction()
{
	return (1 << dis(gen));
}

TestClient::TestClient(int num)
 : number(num), 
 id("testbot" + std::to_string(num)), pwd("testpwd" + std::to_string(num)),
 battle_seed(-1),
 now_state(STATE_CLOSED)
{
}

void TestClient::DoLogin()
{
	if(!isRunning)
		return;
	if(socket == nullptr)
	{
		now_state = STATE_LOGIN;
		LoginProcess();
	}
	else
	{
		socket->SetCleanUp([this](std::shared_ptr<MyClientSocket> client)
		{
			now_state = STATE_LOGIN;
			LoginProcess();
		});
		Close();
	}
}

void TestClient::DoMatch()
{
	if(!isRunning)
		return;
	if(socket == nullptr)
	{
		now_state = STATE_MATCH;
		AccessProcess(1, std::bind(&TestClient::MatchProcess, this, std::placeholders::_1));
	}
	else
	{
		socket->SetCleanUp([this](std::shared_ptr<MyClientSocket> client)
		{
			now_state = STATE_MATCH;
			AccessProcess(1, std::bind(&TestClient::MatchProcess, this, std::placeholders::_1));
		});
		Close();
	}
}

void TestClient::DoBattle()
{
	if(!isRunning)
		return;
	if(socket == nullptr)
	{
		now_state = STATE_BATTLE;
		AccessProcess(1 + battle_seed, std::bind(&TestClient::BattleProcess, this, std::placeholders::_1));
	}
	else
	{
		socket->SetCleanUp([this](std::shared_ptr<MyClientSocket> client)
		{
			now_state = STATE_BATTLE;
			AccessProcess(1 + battle_seed, std::bind(&TestClient::BattleProcess, this, std::placeholders::_1));
		});
		Close();
	}
}

void TestClient::DoRestart()
{
	if(!isRunning)
		return;
	if(socket == nullptr)
	{
		now_state = STATE_RESTART;
		if(doNotRestart)
			return;
		timer->expires_after(std::chrono::milliseconds(millidis(gen)));
		timer->async_wait([this](const boost::system::error_code &ec)
		{
			if(ec.failed())
				return;
			if(doNotRestart)
				return;
			if(!isRunning)
				return;

			LoginProcess();
		});
	}
	else
	{
		socket->SetCleanUp([this](std::shared_ptr<MyClientSocket> client)
		{
			now_state = STATE_RESTART;
			if(doNotRestart)
				return;
			timer->expires_after(std::chrono::milliseconds(millidis(gen)));
			timer->async_wait([this](const boost::system::error_code &ec)
			{
				if(ec.failed())
					return;
				if(doNotRestart)
					return;
				if(!isRunning)
					return;

				LoginProcess();
			});
		});
		Close();
	}
}

void TestClient::Shutdown()
{
	if(socket == nullptr)
	{
		now_state = STATE_CLOSING;
	}
	if(socket != nullptr)
	{
		socket->SetCleanUp([this](std::shared_ptr<MyClientSocket> client)
		{
			now_state = STATE_CLOSING;
		});
		Close();
	}
}

void TestClient::Close()
{
	now_state = STATE_CLOSED;
	if(timer == nullptr)
		timer = std::make_shared<boost::asio::steady_timer>(*pioc);
	else
		timer->cancel();
	if(socket != nullptr)
		socket->Close();
}

void TestClient::LoginProcess()
{
	if(!isRunning)
		return;
	socket = std::make_shared<MyWebsocketClient>(boost::asio::ip::tcp::socket(boost::asio::make_strand(*pioc)));
	socket->Connect(addrs[0].first, addrs[0].second, [this](std::shared_ptr<MyClientSocket> conn_socket, ErrorCode conn_ec)
	{
		if(!conn_ec)
		{
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(conn_ec, __STACKINFO__)));
			RecordError(__STACKINFO__, "Failed Connect : " + conn_ec.message_code());
			DoRestart();
			return;
		}

		conn_socket->KeyExchange([this](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
		{
			if(!ke_ec)
			{
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed KeyExchange : " + ke_ec.message_code());
				DoRestart();
				return;
			}
			ByteQueue loginReq = ByteQueue::Create<byte>(REQ_LOGIN);
			loginReq.push<Hash_t>(Hasher::sha256((const unsigned char*)(pwd.c_str()), pwd.length()));
			loginReq.push<std::string>(id);

			ke_socket->SetTimeout(LoginTimeout, [this](std::shared_ptr<MyClientSocket> socket)
			{
				RecordError(__STACKINFO__, "Timeout at Login");
				socket->Close();
			});
			AuthProcess(ke_socket);
			auto send_ec = ke_socket->Send(loginReq);
			if(!send_ec)
			{
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed Send Login : " + send_ec.message_code());
				DoRestart();
				return;
			}
		});
	});
}

void TestClient::AccessProcess(int server, std::function<void(std::shared_ptr<MyClientSocket>)> handler)
{
	if(!isRunning)
		return;
	socket = std::make_shared<MyWebsocketClient>(boost::asio::ip::tcp::socket(boost::asio::make_strand(*pioc)));
	socket->Connect(addrs[server].first, addrs[server].second, [this, handler](std::shared_ptr<MyClientSocket> conn_socket, ErrorCode conn_ec)
	{
		if(!conn_ec)
		{
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(conn_ec, __STACKINFO__)));
			RecordError(__STACKINFO__, "Failed Connect : " + conn_ec.message_code());
			DoRestart();
			return;
		}

		conn_socket->KeyExchange([this, handler](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
		{
			if(!ke_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed KeyExchange : " + ke_ec.message_code());
				this->DoRestart();
				return;
			}

			handler(ke_socket);
			auto send_ec = ke_socket->Send(ByteQueue::Create<Hash_t>(cookie));
			if(!send_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed Access : " + send_ec.message_code());
				this->DoRestart();
				return;
			}
		});
	});
}

void TestClient::AuthProcess(std::shared_ptr<MyClientSocket> target_socket)
{
	if(!isRunning)
		return;
	target_socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		client->CancelTimeout();
		if(!recv_ec)
		{
			if(!client->isNormalClose(recv_ec))
			{
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(recv_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed Login : " + recv_ec.message_code());
			}
			if(now_state == STATE_LOGIN)
				DoRestart();
			else
				RecordError(__STACKINFO__, "Late Close at " + GetStateName(STATE_LOGIN) + " to " + GetStateName(now_state));
			return;
		}

		byte header = packet.pop<byte>();
		switch(header)
		{
			case ANS_HEARTBEAT:		//연결 확인
				break;
			case SUCCESS:
			case ERR_EXIST_ACCOUNT_MATCH:
			case ERR_EXIST_ACCOUNT_BATTLE:
				cookie = packet.pop<Hash_t>();
				if(header == ERR_EXIST_ACCOUNT_BATTLE)
					battle_seed = packet.pop<Seed_t>();
				else
					battle_seed = -1;

				if(battle_seed < 0)
					DoMatch();
				else
					DoBattle();
				return;
			case ERR_NO_MATCH_ACCOUNT:
				{	//Try Register
					ByteQueue regReq = ByteQueue::Create<byte>(REQ_REGISTER);
					regReq.push<Hash_t>(Hasher::sha256((const unsigned char*)(this->pwd.c_str()), this->pwd.length()));
					regReq.push<std::string>(id);
					client->SetTimeout(LoginTimeout, [this](std::shared_ptr<MyClientSocket> client)
					{
						RecordError(__STACKINFO__, "Timeout at Login");
						client->Close();
					});
					auto send_ec = client->Send(regReq);
					if(!send_ec)
					{
						parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
						RecordError(__STACKINFO__, "Failed Register : " + send_ec.message_code());
						DoRestart();
						return;
					}
				}
				break;
			default:
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed from Server " + std::to_string(header));
				DoRestart();
				return;
		}
		AuthProcess(client);
	});
}

void TestClient::MatchProcess(std::shared_ptr<MyClientSocket> target_socket)
{
	if(!isRunning)
		return;
	target_socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		if(!recv_ec)
		{
			if(!client->isNormalClose(recv_ec))
			{
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(recv_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed Match : " + recv_ec.message_code());
			}
			if(now_state == STATE_MATCH)
				DoRestart();
			else
				RecordError(__STACKINFO__, "Late Close at " + GetStateName(STATE_MATCH) + " to " + GetStateName(now_state));
			return;
		}

		byte header = packet.pop<byte>();
		switch(header)
		{
			case ANS_HEARTBEAT:		//연결 확인
				break;
			case GAME_PLAYER_INFO:
				nickname = packet.pop<Achievement_ID_t>();
				win = packet.pop<int>();
				draw = packet.pop<int>();
				loose = packet.pop<int>();

				{
					auto send_ec = client->Send(ByteQueue::Create<byte>(REQ_STARTMATCH));
					if(!send_ec)
					{
						parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
						RecordError(__STACKINFO__, "Failed Enqueue : " + send_ec.message_code());
						DoRestart();
						return;
					}
				}
				break;
			case ANS_MATCHMADE:
				battle_seed = packet.pop<int>();
				DoBattle();
				return;
			case ERR_PROTOCOL_VIOLATION:	//프로토콜 위반
			case ERR_DB_FAILED:				//db query 실패
			case ERR_NO_MATCH_ACCOUNT:		//cookie 유실
			case ERR_DUPLICATED_ACCESS:		//타 host에서 접속
			default:
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed from Server " + std::to_string(header));
				DoRestart();
				return;
		}
		MatchProcess(client);
	});
}

void TestClient::BattleProcess(std::shared_ptr<MyClientSocket> target_socket)
{
	if(!isRunning)
		return;
	target_socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode recv_ec)
	{
		if(!recv_ec)
		{
			if(!client->isNormalClose(recv_ec))
			{
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(recv_ec, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed Battle : " + recv_ec.message_code());
			}
			if(now_state == STATE_BATTLE)
				DoRestart();
			else
				RecordError(__STACKINFO__, "Late Close at " + GetStateName(STATE_BATTLE) + " to " + GetStateName(now_state));
			return;
		}

		byte header = packet.pop<byte>();
		switch(header)
		{
			case ANS_HEARTBEAT:		//연결 확인
				break;
			case GAME_PLAYER_INFO_NAME:
				//TODO : Check if information is correct
				nickname = packet.pop<Achievement_ID_t>();
				win = packet.pop<int>();
				draw = packet.pop<int>();
				loose = packet.pop<int>();

				enemy_nickname = packet.pop<Achievement_ID_t>();
				enemy_win = packet.pop<int>();
				enemy_draw = packet.pop<int>();
				enemy_loose = packet.pop<int>();
				break;
			case GAME_PLAYER_DISCONNEDTED:	//상대 연결 끊어짐
				break;
			case GAME_PLAYER_ALL_CONNECTED:	//상대 접속
			case GAME_ROUND_RESULT:
				round = packet.pop<int>();
				if(round >= 0)
				{
					action = packet.pop<byte>();
					health = packet.pop<int>();
					energy = packet.pop<int>();
					
					enemy_action = packet.pop<byte>();
					enemy_health = packet.pop<int>();
					enemy_energy = packet.pop<int>();
				}
				{
					int next_action = GetRandomAction();
					ByteQueue actionreq = ByteQueue::Create<byte>(REQ_GAME_ACTION);
					actionreq.push<int>(next_action);
					auto send_ec = client->Send(actionreq);
					if(!send_ec)
					{
						parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
						RecordError(__STACKINFO__, "Failed Action : " + send_ec.message_code());
						DoRestart();
						return;
					}
				}
				break;
			case GAME_FINISHED_WIN:
			case GAME_FINISHED_DRAW:
			case GAME_FINISHED_LOOSE:
			case GAME_CRASHED:
				if(header == GAME_CRASHED)
					RecordError(__STACKINFO__, "Game Crashed");
				DoMatch();
				return;
			case GAME_PLAYER_ACHIEVE:	//도전과제 달성.
				{
					Achievement_ID_t achieve = packet.pop<Achievement_ID_t>();
				}
				break;
			case ERR_PROTOCOL_VIOLATION:	//프로토콜 위반
			case ERR_NO_MATCH_ACCOUNT:		//cookie 유실
			case ERR_DUPLICATED_ACCESS:		//타 host에서 접속
			default:
				parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
				RecordError(__STACKINFO__, "Failed from Server " + std::to_string(header));
				DoRestart();
				return;
		}
		BattleProcess(client);
	});
}

int TestClient::GetId()
{
	return number;
}

Seed_t TestClient::GetSeed()
{
	return battle_seed;
}

int TestClient::GetState()
{
	return now_state;
}

bool TestClient::isConnected()
{
	if(socket == nullptr)
		return false;
	return socket->is_open();
}

void TestClient::RecordError(std::string file, std::string func, int line, std::string reason)
{
	std::unique_lock lk(last_error_mtx);
	last_error = reason + " at " + func + " in " + file + ":" + std::to_string(line);
}

std::string TestClient::GetLastError() const
{
	return last_error;
}

void TestClient::ClearLastError()
{
	if(last_error_mtx.try_lock())
	{
		last_error = "";
		last_error_mtx.unlock();
	}
}

std::string TestClient::GetStateName(int state)
{
	switch(state)
	{
		case STATE_CLOSED:	return "Closed";
		case STATE_RESTART:	return "Restart";
		case STATE_LOGIN:	return "Login";
		case STATE_MATCH:	return "Match";
		case STATE_BATTLE:	return "Batle";
	}
	return "Unknown";
}