#include "MyGetOpt.hpp"

struct option MyCommon::MyGetOpt::long_options[] = 
{
	{"daemon", no_argument, 0, 'd'},		//세번째 인수가 변수 주소(int*)면 해당 변수 주소에 네번째 인수값을 넣어주고, getopt_long()은 0을 리턴한다.
	{"pidfile", required_argument, 0, 'p'},	//세번째 인수가 null이면 getopt_long()에서 네번째 인수값을 리턴한다.
	{"configfile", required_argument, 0, 'c'},
	{"Log", required_argument, 0, 'L'},
	{0, 0, 0, 0}
};

MyCommon::MyGetOpt::MyGetOpt()
{
	daemon_flag = false;
	pid_flag = false;
	pid_path = "";
	conf_path = "";
}

bool MyCommon::MyGetOpt::splitarg(std::string input, std::string &var, std::string &value)
{
	int ptr = input.find('=');
	if(ptr <= 0 || ptr == std::string::npos || ptr >= input.length())
		return false;

	var = input.substr(0, ptr);
	value = input.substr(ptr + 1);

	return true;
}

MyCommon::MyGetOpt::MyGetOpt(int argc, char *argv[]) : MyGetOpt()
{
	GetOpt(argc, argv);
}

void MyCommon::MyGetOpt::GetOpt(int argc, char *argv[])
{
	int option_index = 0;
	int c = 0;
	while((c = getopt_long(argc, argv, "dp:c:L:", long_options, &option_index)) != -1)
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
						throw MyExcepts("There is no Argument : " + std::string(argv[optind-1]), __STACKINFO__);
					std::string var, value;
					if(!splitarg(optarg, std::ref(var), std::ref(value)))
						throw MyExcepts("Parse Failed : " + std::string(argv[optind-1]), __STACKINFO__);
					int port = std::atoi(value.c_str());
					MyLogger::ConfigPort(MyLogger::GetType(var), port);
				}
				break;
			case '?':
				throw MyExcepts("Unknown Argument : " + std::string(argv[optind-1]), __STACKINFO__);
			default:
				throw MyExcepts("getopt_long gets error(" + std::to_string(c) + ") : " + std::string(argv[optind-1]), __STACKINFO__);
		}
	}
}