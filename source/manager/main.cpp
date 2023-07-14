#include <iostream>
#include <map>
#include "MgrOpt.hpp"
#include "ManagerServiceClient.hpp"
#include "StackTraceExcept.hpp"

//Inquiry among Servers
static constexpr byte INQ_QUIT = 0;				//프로그램 종료
static constexpr byte INQ_ACCOUNT_CHECK = 131;	//계정 접속/만료 여부 확인.
static constexpr byte INQ_USAGE = 134;			//사용량(서버 내 자원) 확인.
static constexpr byte INQ_CLIENTUSAGE = 135;	//사용량(클라이언트 접속량) 확인. 동시접속자 수와 동일
static constexpr byte INQ_CONNUSAGE = 136;		//사용량(Connector 접속량) 확인.

using mylib::utils::ErrorCodeExcept;

std::map<std::string, byte> cmdtable = {
	{"client", INQ_CLIENTUSAGE},
	{"connect", INQ_CONNUSAGE},
	{"usage", INQ_USAGE},
	{"account", INQ_ACCOUNT_CHECK},
	{"help", ERR_DB_FAILED},
	{"quit", INQ_QUIT}
};
MgrOpt opt;
std::queue<std::string> commands;

void ShowCommands();
void ShowHowToUse(std::string);

byte GetRequest(std::string command)
{
	auto header = cmdtable.find(command);
	if(header == cmdtable.end())
	{
		static const auto cmp = [](const std::string exp, const std::string ctr) -> size_t
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
		};
		std::map<byte, size_t> compare;
		for(auto &pair : cmdtable)
			compare.insert({pair.second, cmp(command, pair.first)});
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

void Process(ManagerServiceClient* client, std::queue<std::string> &commands)
{
	auto command = commands.front();
	auto request = GetRequest(command);
	commands.pop();

	switch(request)
	{
		case INQ_USAGE:
			{
				auto answer = client->GetUsage();
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);
				
				std::cout << "[Mgr] : Server Usage : " << *answer << std::endl;
			}
			break;
		case INQ_ACCOUNT_CHECK:
			{
				if(commands.empty())
				{
					ShowCommands();
					throw StackTraceExcept(command + " : Argument Required", __STACKINFO__);
				}
				auto argument = commands.front();
				commands.pop();
				Account_ID_t id = std::stoull(argument);
				auto answer = client->CheckAccount(id);
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);
				
				if(*answer)
					std::cout << "[Mgr] : Account " << std::to_string(id) << " Found" << std::endl;
				else
					std::cout << "[Mgr] : Account " << std::to_string(id) << " Not Found" << std::endl;
			}
			break;
		case INQ_CLIENTUSAGE:
			{
				auto answer = client->GetClientUsage();
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);
				
				std::cout << "[Mgr] : Clients : " << *answer << std::endl;
			}
			break;
		case INQ_CONNUSAGE:
			{
				auto answer = client->GetConnectUsage();
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);

				std::cout << "[Mgr] : Connections" << std::endl;
				if(answer->size() > 0)
				{
					for(auto &pair : *answer)
						std::cout << pair.first << " : " << pair.second << std::endl;
				}
				else
					std::cout << "No Connection" << std::endl;
			}
			break;
		case ERR_DB_FAILED:
			std::cout << "[Mgr] : ";
			ShowCommands();
			break;
		case 0:
			opt.shell_mode = false;
			break;
		default:
			ShowCommands();
			throw StackTraceExcept("Unknown Command \"" + command + "\"", __STACKINFO__);
	}
}

int main(int argc, char *argv[])
{
	try
	{
		opt.GetArgs(argc, argv);
		if(opt.commands.empty())
			opt.shell_mode = true;
		while(!opt.commands.empty())
		{
			commands.push(opt.commands.front());
			opt.commands.pop();
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		ShowHowToUse(std::string(argv[0]));
		return -1;
	}

	ManagerServiceClient client(opt.addr, opt.port);

	std::string input = "";
	do
	{
		static std::string trimmer = " \t\n\r\f\v";
		try
		{
			for(int split_pos = input.find_first_of(trimmer);split_pos != std::string::npos;split_pos = input.find_first_of(trimmer))
			{
				std::string split = input.substr(0, split_pos);
				if(split.length() > 0)
					commands.push(split);
				input.erase(0, split_pos + 1);
			}
			if(input.length() > 0)
				commands.push(input);

			while(!commands.empty())
				Process(&client, std::ref(commands));
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		input.clear();
		if(!opt.shell_mode)
			break;
		std::cout << "[Command] : ";
	}
	while(std::getline(std::cin, input) && input.length() > 0);
	
	return 0;
}

void ShowHowToUse(std::string prog_name)
{
	std::cout << "[Usage] : " << prog_name << " [-<option> <arg>] [<command> ...]" << std::endl << std::endl;
	std::cout << "<Options>" << std::endl;
	std::cout << "	-a, --address		Address of target machine. Default Value is \"localhost\"." << std::endl;
	std::cout << "	-p, --port			Port of target machine. Default Value is \"52431\"." << std::endl;
	std::cout << "	-s, --shell			Shell mode. Command like shell. If there are no command argument, it will on automatically." << std::endl << std::endl;
	ShowCommands();
}

void ShowCommands()
{
	std::cout << "<Commands>" << std::endl;
	std::cout << "	client			Get How many clients are connected." << std::endl;
	std::cout << "	connect			Get How many machines are connected with as what name." << std::endl;
	std::cout << "	usage			Get Usage of the machine. It will only works for \'battle\' Server." << std::endl;
	std::cout << "	account <id>	Find Account if it is connected." << std::endl;
}