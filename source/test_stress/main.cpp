#include <stdio.h>
#include <iostream>
#include <boost/asio/thread_pool.hpp>
#include <thread>
#include <map>
#include "TestOpt.hpp"
#include "TestClient.hpp"
#include "ThreadExceptHandler.hpp"

static constexpr byte INQ_QUIT = 0;
static constexpr byte INQ_INSERT_CLIENT = 1;
static constexpr byte INQ_SHOW_CLIENT = 2;
static constexpr byte INQ_BRIEF_CLIENTS = 3;
static constexpr byte INQ_ERROR = 4;
static constexpr byte INQ_RESTART = 5;
static constexpr byte INQ_DELAY = 6;
static constexpr byte INQ_SCORE = 7;
static constexpr byte INQ_HELP = 8;
static constexpr byte INQ_EXIT = 9;

using mylib::threads::ThreadExceptHandler;

std::map<std::string, byte> cmdtable = {
	{"insert", INQ_INSERT_CLIENT},
	{"show", INQ_SHOW_CLIENT},
	{"brief", INQ_BRIEF_CLIENTS},
	{"error", INQ_ERROR},
	{"restart", INQ_RESTART},
	{"delay", INQ_DELAY},
	{"help", INQ_HELP},
	{"quit", INQ_QUIT},
	{"exit", INQ_EXIT}
};

void ShowCommands();
void ShowHowToUse(std::string);

size_t str_cmp(const std::string exp, const std::string ctr)
{
	size_t length = std::min(exp.size(), ctr.size());
	size_t fitCount = 0;
	for(size_t i = 0;i < length; i++)
	{
		if(exp[i] != ctr[i])
			break;
		fitCount++;
	}
	return fitCount;
}

byte GetRequest(std::string command)
{
	auto header = cmdtable.find(command);
	if(header == cmdtable.end())
	{
		std::map<byte, size_t> compare;
		for(auto &pair : cmdtable)
			compare.insert({pair.second, str_cmp(command, pair.first)});
		static const auto compare_cmp = [](std::pair<byte, size_t> lhs, std::pair<byte, size_t> rhs) -> bool
		{ return lhs.second < rhs.second; };
		auto answer = std::max_element(compare.begin(), compare.end(), compare_cmp);
		size_t count = 0;
		for(auto &pair : compare)
		{
			if(pair.second == answer->second)
				count++;
		}
		if(count > 1)
			return ERR_PROTOCOL_VIOLATION;
		return answer->first;
	}
	return header->second;
}

std::queue<std::string> ParseLine(std::string line)
{
	static std::string trimmer = " \t\n\r\f\v";
	std::queue<std::string> result;

	for(int split_pos = line.find_first_of(trimmer);split_pos != std::string::npos;split_pos = line.find_first_of(trimmer))
	{
		std::string split = line.substr(0, split_pos);
		if(split.length() > 0)
			result.push(split);
		line.erase(0, split_pos + 1);
	}
	if(line.length() > 0)
		result.push(line);

	return result;
}

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
		
		TestClient::AddAddr(opt.addr, 54322);	//auth
		TestClient::AddAddr(opt.addr, 54323);	//match
		TestClient::AddAddr(opt.addr, 54324);	//battle1
		//TestClient::AddAddr(opt.addr, 54325);	//battle2
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

	std::queue<std::string> command;
	std::queue<std::string> recent_command;
	while(!kill_signal)
	{
		std::string input;
		std::getline(std::cin, input);

		command = ParseLine(input);
		if(command.empty())
			command = recent_command;
		recent_command = command;
		
		std::string cmdstr = command.front();
		command.pop();
		byte request = GetRequest(cmdstr);
		switch(request)
		{
			case INQ_INSERT_CLIENT:
				{
					int count = 1;
					if(!command.empty())
					{
						try
						{
							count = std::stoi(command.front());
							command.pop();
						}
						catch(...)
						{
							count = 1;
						}
					}
					for(int i = 0;i<count;i++)
					{
						auto client = std::make_shared<TestClient>(id++);
						client->DoLogin();
						clients.push_back(client);
					}
					g_Show("Insert " + std::to_string(count) + " Clients.");
				}
				break;
			case INQ_SHOW_CLIENT:	//Show all client's state
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
			case INQ_ERROR:	//Show All client's last error
				{
					unsigned int count = 0;
					for(auto &client : clients)
					{
						std::string txt_id = std::to_string(client->GetId());
						std::string last_error = client->GetLastError();
						if(last_error.length() <= 0)
							continue;
						client->ClearLastError();
						g_Show(txt_id + " : " + last_error);
						count++;
					}
					if(count <= 0)
						g_Show("There is no Error");
				}
				break;
			case INQ_BRIEF_CLIENTS:	//Brief all client's state
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
			case INQ_RESTART:	//Restart Clients
				for(auto &client : clients)
				{
					if(client->GetState() != 0 && !client->isConnected())
						client->DoRestart();
				}
				break;
			case INQ_DELAY:
				{
					int mode = 0;
					if(!command.empty())
					{
						if(str_cmp(command.front(), "max") > 0)
							mode = 1;	//max mode
					}
					double delay_auth = 0.0;
					double delay_match = 0.0;
					double delay_battle = 0.0;

					std::function<void(double, double, double)> calcfunc;
					switch(mode)
					{
						case 0:
							calcfunc = [&](double auth, double match, double battle)
							{
								delay_auth += auth;
								delay_match += match;
								delay_battle += battle;
							};
							break;
						case 1:
							calcfunc = [&](double auth, double match, double battle)
							{
								delay_auth = std::max(delay_auth, auth);
								delay_match = std::max(delay_match, match);
								delay_battle = std::max(delay_battle, battle);
							};
							break;
					}
					for(auto &client : clients)
					{
						auto [auth, match, battle] = client->GetDelays();
						calcfunc(auth, match, battle);
					}

					std::string mode_str;
					if(clients.size() > 0)
					{
						switch(mode)
						{
							case 0:
								mode_str = "Average";
								delay_auth /= clients.size();
								delay_match /= clients.size();
								delay_battle /= clients.size();
								break;
							case 1:
								mode_str = "Max";
								break;
						}
					}
					g_Show("Delay Result : " + mode_str);
					g_Show("Auth Delay : " + std::to_string(delay_auth) + "sec");
					g_Show("Match Delay : " + std::to_string(delay_match) + "sec");
					g_Show("Battle Delay : " + std::to_string(delay_battle) + "sec");
				}
				break;
			case INQ_HELP:
				ShowCommands();
				break;
			case INQ_QUIT:
			case INQ_EXIT:
				kill_signal = true;
				break;
			default:
				g_Show("Unknown Input : " + cmdstr);
				break;
		}
	}

	TestClient::isRunning = false;
	TestClient::SetRestart(true);
	kill_signal = true;
	for(auto iter = clients.begin();iter != clients.end();iter++)
	{
		if((*iter)->isConnected())
			(*iter)->Shutdown();	//TODO : Close하기 전, 결과 출력하기.
	}

	threads.join();
	ioc.stop();
	clients.clear();

	return 0;
}

void ShowCommands()
{
	std::cout << "<Commands>" << std::endl;
	std::cout << "	insert <num>	Insert a New Client." << std::endl;
	std::cout << "	show			Show Every Client with its status." << std::endl;
	std::cout << "	brief			Show brief clients' status." << std::endl;
	std::cout << "	error			Show Recent Error Message for every error-occured clients." << std::endl;
	std::cout << "	restart			Restart all stopped clients." << std::endl;
	std::cout << "	delay			Show Average Delay for each server." << std::endl;
	std::cout << "	help			Show Command Instructions." << std::endl;
	std::cout << "	quit			Exit Program" << std::endl;
}