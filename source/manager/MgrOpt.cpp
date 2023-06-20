#include "MgrOpt.hpp"

struct option MgrOpt::long_options[] = 
{
	{"address", required_argument, 0, 'a'},
	{"port", required_argument, 0, 'p'},
	{"shell", no_argument, 0, 's'},
	{0, 0, 0, 0}
};

MgrOpt::MgrOpt()
{
	ClearOpt();
}

void MgrOpt::ClearOpt()
{
	addr = "localhost";
	port = 52431;	//default port
	remote_keyword = "manager";
	shell_mode = false;
	while(!commands.empty())
		commands.pop();
}

void MgrOpt::GetArgs(int argc, char *argv[])
{
	int option_index = 0;
	int c = 0;
	while((c = getopt_long(argc, argv, "a:p:s", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'a':
				addr = std::string(optarg);
				break;
			case 'p':
				port = std::atoi(optarg);
				break;
			case 's':
				shell_mode = true;
				break;
			case '?':
				throw StackTraceExcept("Unknown Argument : " + std::string(argv[optind-1]), __STACKINFO__);
			default:
				throw StackTraceExcept("getopt_long gets error(" + std::to_string(c) + ") : " + std::string(argv[optind-1]), __STACKINFO__);
		}
	}

	if(optind >= argc)
		throw StackTraceExcept("No Remote Keyword", __STACKINFO__);
	
	remote_keyword = std::string(argv[optind++]);
	
	while(optind < argc)
		commands.push(std::string(argv[optind++]));
	
	if(commands.empty())
		shell_mode = true;
}