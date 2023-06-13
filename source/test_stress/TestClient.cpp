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

TestClient::~TestClient()
{
	Close();
}

void TestClient::DoLogin()
{
	Close();

	now_state = STATE_LOGIN;

	socket = std::make_shared<MyWebsocketClient>(boost::asio::ip::tcp::socket(*pioc));
	socket->Connect(addrs[0].first, addrs[0].second, [this](std::shared_ptr<MyClientSocket> conn_socket, ErrorCode conn_ec)
	{
		if(!conn_ec)
		{
			this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(conn_ec, __STACKINFO__)));
			this->DoRestart();
			return;
		}

		conn_socket->KeyExchange([this](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
		{
			if(!ke_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				this->DoRestart();
				return;
			}
			ByteQueue loginReq = ByteQueue::Create<byte>(REQ_LOGIN);
			loginReq.push<Hash_t>(Hasher::sha256((const unsigned char*)(this->pwd.c_str()), this->pwd.length()));
			loginReq.push<std::string>(id);

			ke_socket->SetTimeout(LoginTimeout);
			ke_socket->StartRecv(std::bind(&TestClient::LoginProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			auto send_ec = ke_socket->Send(loginReq);
			if(!send_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
				this->DoRestart();
				return;
			}
		});
	});
}

void TestClient::DoMatch()
{
	Close();
	
	now_state = STATE_MATCH;

	std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handle = std::bind(&TestClient::MatchProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	std::function<void()> process = std::bind(&TestClient::AccessProcess, this, 1, handle);
	boost::asio::post(*pioc, process);
}

void TestClient::DoBattle()
{
	Close();

	now_state = STATE_BATTLE;

	std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handle = std::bind(&TestClient::BattleProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	std::function<void()> process = std::bind(&TestClient::AccessProcess, this, 1 + battle_seed, handle);
	boost::asio::post(*pioc, process);
}

void TestClient::DoRestart()
{
	Close();

	now_state = STATE_RESTART;

	if(doNotRestart)
		return;

	timer->expires_after(std::chrono::milliseconds(millidis(gen)));
	timer->async_wait([this](const boost::system::error_code &ec)
	{
		if(ec.failed())
			return;
		if(this->doNotRestart)
			return;

		this->DoLogin();
	});
}

void TestClient::Close()
{
	now_state = STATE_CLOSED;
	if(socket != nullptr)
	{
		//socket->StartRecv([](std::shared_ptr<MyClientSocket> close_socket, ByteQueue answer, ErrorCode ec){});
		socket->CancelTimeout();
		if(socket->is_open())
			socket->Close();
	}
	if(timer == nullptr)
		timer = std::make_shared<boost::asio::steady_timer>(*pioc);
	else
		timer->cancel();
}

void TestClient::LoginProcess(std::shared_ptr<MyClientSocket> socket, ByteQueue answer, ErrorCode ec)
{
	socket->CancelTimeout();
	if(!ec)
	{
		if(now_state == STATE_LOGIN)
		{
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ec, __STACKINFO__)));
			DoRestart();
			return;
		}
		if(!socket->isNormalClose(ec))
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ec, __STACKINFO__)));
		return;
	}

	byte header = answer.pop<byte>();
	switch(header)
	{
		case SUCCESS:
		case ERR_EXIST_ACCOUNT_MATCH:
		case ERR_EXIST_ACCOUNT_BATTLE:
			cookie = answer.pop<Hash_t>();
			if(header == ERR_EXIST_ACCOUNT_BATTLE)
				battle_seed = answer.pop<Seed_t>();
			else
				battle_seed = -1;

			if(battle_seed < 0)
				DoMatch();
			else
				DoBattle();
			break;
		case ERR_NO_MATCH_ACCOUNT:
			{	//Try Register
				ByteQueue regReq = ByteQueue::Create<byte>(REQ_REGISTER);
				regReq.push<Hash_t>(Hasher::sha256((const unsigned char*)(this->pwd.c_str()), this->pwd.length()));
				regReq.push<std::string>(id);
				socket->SetTimeout(LoginTimeout);
				auto send_ec = socket->Send(regReq);
				if(!send_ec)
				{
					parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
					DoRestart();
				}
			}
			break;
		default:
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
			DoRestart();
			break;
	}
}

void TestClient::AccessProcess(int server, std::function<void(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode)> handler)
{
	socket = std::make_shared<MyWebsocketClient>(boost::asio::ip::tcp::socket(*pioc));
	socket->Connect(addrs[server].first, addrs[server].second, [this, handler](std::shared_ptr<MyClientSocket> conn_socket, ErrorCode conn_ec)
	{
		if(!conn_ec)
		{
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(conn_ec, __STACKINFO__)));
			DoRestart();
			return;
		}

		conn_socket->KeyExchange([this, handler](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
		{
			if(!ke_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				this->DoRestart();
				return;
			}

			ke_socket->StartRecv(handler);
			auto send_ec = ke_socket->Send(ByteQueue::Create<Hash_t>(cookie));
			if(!send_ec)
			{
				this->parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ke_ec, __STACKINFO__)));
				this->DoRestart();
				return;
			}
		});
	});
}

void TestClient::MatchProcess(std::shared_ptr<MyClientSocket> socket, ByteQueue answer, ErrorCode ec)
{
	if(!ec)
	{
		if(!socket->isNormalClose(ec))
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ec, __STACKINFO__)));
		if(now_state == STATE_MATCH)
			DoRestart();
		return;
	}

	byte header = answer.pop<byte>();
	switch(header)
	{
		case ERR_PROTOCOL_VIOLATION:	//프로토콜 위반
		case ERR_DB_FAILED:				//db query 실패
		case ERR_NO_MATCH_ACCOUNT:		//cookie 유실
		case ERR_DUPLICATED_ACCESS:		//타 host에서 접속
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
			DoRestart();
			break;
		case GAME_PLAYER_INFO:
			nickname = answer.pop<Achievement_ID_t>();
			win = answer.pop<int>();
			draw = answer.pop<int>();
			loose = answer.pop<int>();

			{
				auto send_ec = socket->Send(ByteQueue::Create<byte>(REQ_STARTMATCH));
				if(!send_ec)
				{
					parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
					DoRestart();
				}
			}
			break;
		case ANS_MATCHMADE:
			battle_seed = answer.pop<int>();
			DoBattle();
			break;
		default:
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
			DoRestart();
			break;
	}
}

void TestClient::BattleProcess(std::shared_ptr<MyClientSocket> socket, ByteQueue answer, ErrorCode ec)
{
	if(!ec)
	{
		if(!socket->isNormalClose(ec))
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ec, __STACKINFO__)));
		if(now_state == STATE_BATTLE)
			DoRestart();
		return;
	}

	byte header = answer.pop<byte>();
	switch(header)
	{
		case ERR_PROTOCOL_VIOLATION:	//프로토콜 위반
		case ERR_NO_MATCH_ACCOUNT:		//cookie 유실
		case ERR_DUPLICATED_ACCESS:		//타 host에서 접속
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
			DoRestart();
			break;
		case GAME_PLAYER_INFO_NAME:
			//TODO : Check if information is correct
			nickname = answer.pop<Achievement_ID_t>();
			win = answer.pop<int>();
			draw = answer.pop<int>();
			loose = answer.pop<int>();

			enemy_nickname = answer.pop<Achievement_ID_t>();
			enemy_win = answer.pop<int>();
			enemy_draw = answer.pop<int>();
			enemy_loose = answer.pop<int>();
			break;
		case GAME_PLAYER_DISCONNEDTED:	//상대 연결 끊어짐
			break;
		case GAME_PLAYER_ALL_CONNECTED:	//상대 접속
		case GAME_ROUND_RESULT:
			round = answer.pop<int>();
			if(round >= 0)
			{
				action = answer.pop<byte>();
				health = answer.pop<int>();
				energy = answer.pop<int>();
				
				enemy_action = answer.pop<byte>();
				enemy_health = answer.pop<int>();
				enemy_energy = answer.pop<int>();
			}
			{
				int next_action = GetRandomAction();
				ByteQueue actionreq = ByteQueue::Create<byte>(REQ_GAME_ACTION);
				actionreq.push<int>(next_action);
				auto send_ec = socket->Send(actionreq);
				if(!send_ec)
				{
					parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(send_ec, __STACKINFO__)));
					DoRestart();
				}
			}
			break;
		case GAME_FINISHED_WIN:
		case GAME_FINISHED_DRAW:
		case GAME_FINISHED_LOOSE:
		case GAME_CRASHED:
			DoMatch();
			break;
		case GAME_PLAYER_ACHIEVE:	//도전과제 달성.
			{
				Achievement_ID_t achieve = answer.pop<Achievement_ID_t>();
			}
			break;
		default:
			parent->ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(header, __STACKINFO__)));
			DoRestart();
			break;
	}
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