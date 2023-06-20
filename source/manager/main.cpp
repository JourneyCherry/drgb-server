#include <iostream>
#include <map>
#include "MgrOpt.hpp"
#include "MyConnectorPool.hpp"

std::map<std::string, byte> cmdtable = {
	{"client", INQ_CLIENTUSAGE},
	{"connector", INQ_CONNUSAGE},
	{"usage", INQ_USAGE},
	{"sessions", INQ_SESSIONS},
	{"account", INQ_ACCOUNT_CHECK}
};

std::string keyword;
MyConnectorPool pool("drgbmgr", 1);

byte GetRequest(std::string command)
{
	auto header = cmdtable.find(command);
	if(header == cmdtable.end())
		return ERR_PROTOCOL_VIOLATION;
	return header->second;
}

void ShowResult(byte request, ByteQueue answer)
{
	if(answer.Size() <= 0)
	{
		std::cout << "[Mgr] : Zero-byte Received" << std::endl;
		return;
	}
	
	byte header = answer.pop<byte>();
	switch(header)
	{
		case SUCCESS:
			switch(request)
			{
				case INQ_CLIENTUSAGE:
					{
						std::cout << "[Mgr] : Client Usage" << std::endl;
						std::cout << "TCP Client : " << std::to_string(answer.pop<size_t>()) << std::endl;
						std::cout << "WEB Client : " << std::to_string(answer.pop<size_t>()) << std::endl;
					}
					break;
				case INQ_CONNUSAGE:
					{
						std::cout << "[Mgr] : Connector Usage" << std::endl;
						std::cout << "==Connectee==" << std::endl;
						size_t tee = answer.pop<size_t>();
						for(size_t i = 0;i<tee;i++)
						{
							size_t len = answer.pop<size_t>();
							std::string keyword = answer.popstr(len, false);
							size_t count = answer.pop<size_t>();
							std::cout << keyword << " : " << count << std::endl;
						}
						std::cout << "==Connector==" << std::endl;
						size_t tor = answer.pop<size_t>();
						for(size_t i = 0;i<tor;i++)
						{
							size_t len = answer.pop<size_t>();
							std::string keyword = answer.popstr(len, false);
							int state = answer.pop<int>();
							std::string state_str = "";
							if(state < 0)
								state_str = "Disconnected";
							else if(state == 0)
								state_str = "Connected";
							else
								state_str = "Authorized";
							std::cout << keyword << " : " << state_str << std::endl;
						}
					}
					break;
				case INQ_USAGE:
					std::cout << "[Mgr] : Server Usage : " << std::to_string(answer.pop<size_t>()) << std::endl;
					break;
				case INQ_SESSIONS:
					std::cout << "[Mgr] : Server Sessions : " << std::to_string(answer.pop<size_t>()) << std::endl;
					break;
				default:
					std::cout << "[Mgr] : Unknown Request : " << std::to_string(request) << std::endl;
					break;
			}
			break;
		case ERR_PROTOCOL_VIOLATION:
			std::cout << "[Mgr] : Not Supported Request : " << std::to_string(request) << std::endl;
			break;
		default:
			std::cout << "[Mgr] : Unknown Answer : " << std::to_string(header) << std::endl;
			break;
	}
}

void Process(std::queue<std::string> &commands)
{
	auto command = commands.front();
	commands.pop();
	auto request = GetRequest(command);
	if(request == ERR_PROTOCOL_VIOLATION)
		throw StackTraceExcept("Unknown Command : " + command, __STACKINFO__);
	ByteQueue request_bytes = ByteQueue::Create<byte>(request);
	switch(request)
	{
		case INQ_ACCOUNT_CHECK:
			{
				if(commands.empty())
					throw StackTraceExcept("Argument required", __STACKINFO__);
				Account_ID_t id = std::stoull(commands.front());
				commands.pop();
				request_bytes.push<Account_ID_t>(id);
				auto answer = pool.Request(keyword, request_bytes);
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);
				
				if(answer->Size() <= 0)
					std::cout << "[Mgr] : Zero-byte Received" << std::endl;
				else
				{
					byte result = answer->pop<byte>();
					if(result == SUCCESS)
						std::cout << "[Mgr] : Account " << std::to_string(id) << " Found" << std::endl;
					else if(result == ERR_NO_MATCH_ACCOUNT)
						std::cout << "[Mgr] : Account " << std::to_string(id) << " Not Found" << std::endl;
					else if(result == ERR_PROTOCOL_VIOLATION)
						std::cout << "[Mgr] : Not Supported Request : " << std::to_string(request) << std::endl;
					else
						std::cout << "[Mgr] : Unknown Answer : " << std::to_string(result) << std::endl;
				}
			}
			break;
		default:
			{
				auto answer = pool.Request(keyword, request_bytes);
				if(!answer)
					throw ErrorCodeExcept(answer.error(), __STACKINFO__);

				ShowResult(request, *answer);
			}
			break;
	}
}

int main(int argc, char *argv[])
{
	MgrOpt opt;
	try
	{
		opt.GetArgs(argc, argv);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		//TODO : 사용 가능한 옵션에 대한 설명 출력
		pool.Close();
		return -1;
	}

	try
	{
		pool.Connect(opt.addr, opt.port, opt.remote_keyword, [](ByteQueue request){return ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);});
		
		while(pool.GetAuthorized() < 1)	//Connect 될때까지 기다리기.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));	//TODO : 대기가 아닌 연결 실패 시, 즉시 종료 필요.

		keyword = opt.remote_keyword;

		while(!opt.commands.empty())
			Process(std::ref(opt.commands));
		
		if(!opt.shell_mode)
		{
			pool.Close();
			return 0;
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		pool.Close();
		return -1;
	}

	std::queue<std::string> commands;
	std::string input;
	std::cout << "[Command] : ";
	while(std::getline(std::cin, input) && input.length() > 0)
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

			Process(std::ref(commands));
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		input.clear();
		std::cout << "[Command] : ";
	}

	pool.Close();
	
	return 0;
}