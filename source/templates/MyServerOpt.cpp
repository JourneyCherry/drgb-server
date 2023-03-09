#include "MyServerOpt.hpp"

struct option MyServerOpt::long_options[] = 
{
	{"daemon", no_argument, 0, 'd'},		//세번째 인수가 변수 주소(int*)면 해당 변수 주소에 네번째 인수값을 넣어주고, getopt_long()은 0을 리턴한다.
	{"pidfile", required_argument, 0, 'p'},	//세번째 인수가 null이면 getopt_long()에서 네번째 인수값을 리턴한다.
	{"configfile", required_argument, 0, 'c'},
	{"Log", required_argument, 0, 'L'},
	{"verbose", no_argument, 0, 'v'},
	{0, 0, 0, 0}
};

MyServerOpt::MyServerOpt(int argc, char *argv[])
{
	ClearOpt();
	GetArgs(argc, argv);
}

void MyServerOpt::ClearOpt()
{
	daemon_flag = false;
	pid_flag = false;
	pid_path = "";
	conf_path = "";
	verbose_flag = false;
}

void MyServerOpt::GetArgs(int argc, char *argv[])
{
	int option_index = 0;
	int c = 0;
	while((c = getopt_long(argc, argv, "dp:c:L:v", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 'd':
				daemon_flag = true;
				break;
			case 'p':
				pid_flag = true;
				pid_path = std::string(optarg);
				break;
			case 'c':
				conf_path = std::string(optarg);
		 		break;
			case 'L':
				{
					if(!optarg)
						throw StackTraceExcept("There is no Argument : " + std::string(argv[optind-1]), __STACKINFO__);
					std::string var, value;
					if(!splitarg(optarg, std::ref(var), std::ref(value)))
						throw StackTraceExcept("Parse Failed : " + std::string(argv[optind-1]), __STACKINFO__);
					int port = std::atoi(value.c_str());
					Logger::ConfigPort(Logger::GetType(var), port);
				}
				break;
			case 'v':
				verbose_flag = true;
				break;
			case '?':
				throw StackTraceExcept("Unknown Argument : " + std::string(argv[optind-1]), __STACKINFO__);
			default:
				throw StackTraceExcept("getopt_long gets error(" + std::to_string(c) + ") : " + std::string(argv[optind-1]), __STACKINFO__);
		}
	}
}