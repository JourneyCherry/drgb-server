#include <stdio.h>
#include <iostream>
#include <boost/asio/thread_pool.hpp>
#include <thread>
#include "ConfigParser.hpp"
#include "TestOpt.hpp"
#include "TestClient.hpp"
#include "ThreadExceptHandler.hpp"

using mylib::threads::ThreadExceptHandler;
using mylib::utils::ConfigParser;

class ExceptQueue : public ThreadExceptHandler
{
	public:
		ExceptQueue() : ThreadExceptHandler(){}
		void ThrowIf()
		{
			if(!Exception_Exps.empty())
				WaitForThreadException();
		}
};

bool kill_signal = false;

void signal_handler(int signal_code)
{
	signal(signal_code, SIG_DFL);
	switch(signal_code)
	{
		case SIGTERM:
		case SIGINT:
		case SIGABRT:
			kill_signal = true;
			return;
	}
}

std::mutex g_mtx;
void g_Show(std::string str)
{
	std::unique_lock<std::mutex> lk(g_mtx);
	std::cout << str << std::endl;
}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, signal_handler);

	TestOpt opt;
	try
	{
		opt.GetArgs(argc, argv);
		if(!ConfigParser::ReadFile("../../resources/drgb.conf"))
			throw StackTraceExcept("Read Config file Failed", __STACKINFO__);
		
		TestClient::AddAddr(opt.addr, ConfigParser::GetInt("Auth_ClientPort_Web"));
		TestClient::AddAddr(opt.addr, ConfigParser::GetInt("Match_ClientPort_Web"));
		TestClient::AddAddr(opt.addr, ConfigParser::GetInt("Battle1_ClientPort_Web"));
	}
	catch(const std::exception &e)
	{
		g_Show("[Exception] " + std::string(e.what()));
		return -1;
	}

	unsigned int thread_num = opt.thread_count;

	std::vector<std::shared_ptr<TestClient>> clients;
	ExceptQueue teh;

	boost::asio::io_context ioc(thread_num);
	boost::asio::thread_pool threads(thread_num);
	
	for(int i = 0;i<thread_num;i++)
		boost::asio::post(threads, [&ioc, &teh]()
		{
			while(!kill_signal)
			{
				try
				{
					if(ioc.stopped())
						ioc.restart();
					ioc.run();
				}
				catch(...)
				{
					teh.ThrowThreadException(std::current_exception());
				}
			}
		});

	TestClient::SetInfo(&teh, &ioc);

	int id = 1;
	for(int i = 0;i<opt.client_count;i++)
	{
		auto client = std::make_shared<TestClient>(id++);
		client->DoLogin();
		clients.push_back(client);
	}

	int key_input = 0;
	while(key_input >= 0 && !kill_signal)
	{
		key_input = fgetc(stdin);
		switch(key_input)
		{
			case 'i':
			case 'I':
				{
					auto client = std::make_shared<TestClient>(id++);
					client->DoLogin();
					clients.push_back(client);
				}
				break;
			case 's':
			case 'S':
				//Show all client's state
				for(auto &client : clients)
				{
					std::string txt_id = std::to_string(client->GetId());
					std::string state = "Unknown";
					std::string connected = client->isConnected()?"":"(Disconnected)";
					switch(client->GetState())
					{
						case -1:	state = "Closed";	break;
						case 0:		state = "ReStart";	break;
						case 1:		state = "Login";	break;
						case 2:		state = "Match";	break;
						case 3:		state = "Battle";	break;
					}
					g_Show(txt_id + " : " + state + connected);
				}
				break;
			case 'b':
			case 'B':
				//Brief all client's state
				{
					std::array<int, 5> counters = {0, 0, 0, 0, 0};
					std::array<int, 5> discounters = {0, 0, 0, 0, 0};
					for(auto &client : clients)
					{
						counters[client->GetState() + 1] += 1;
						if(!client->isConnected())
							discounters[client->GetState() + 1] += 1;
					}
					g_Show("Closed : " + std::to_string(counters[0]) + " (" + std::to_string(discounters[0]) + ")");
					g_Show("Restart : " + std::to_string(counters[1]) + " (" + std::to_string(discounters[1]) + ")");
					g_Show("Login : " + std::to_string(counters[2]) + " (" + std::to_string(discounters[2]) + ")");
					g_Show("Match : " + std::to_string(counters[3]) + " (" + std::to_string(discounters[3]) + ")");
					g_Show("Battle : " + std::to_string(counters[4]) + " (" + std::to_string(discounters[4]) + ")");
				}
				break;
			case 'r':
			case 'R':
				for(auto &client : clients)
				{
					if(client->GetState() != 0 && !client->isConnected())
						client->DoRestart();
				}
				break;
			case 'q':
			case 'Q':
				key_input = -1;
				break;
		}
	}

	TestClient::SetRestart(true);
	for(auto iter = clients.begin();iter != clients.end();iter = clients.erase(iter))
		(*iter)->Close();	//TODO : Close하기 전, 결과 출력하기.
	clients.clear();

	kill_signal = true;
	ioc.stop();
	threads.join();

	return 0;
}