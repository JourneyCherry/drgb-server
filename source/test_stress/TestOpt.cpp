#include "TestOpt.hpp"

struct option TestOpt::long_options[] = 
{
	{"thread", required_argument, 0, 't'},
	{"num", required_argument, 0, 'n'},
	{0, 0, 0, 0}
};

TestOpt::TestOpt()
{
	ClearOpt();
}

void TestOpt::ClearOpt()
{
	thread_count = 1;
	client_count = 0;
	addr = "localhost";
	optind = 1;	//getopt_long에서 사용하는 Global Variable 초기화.
}

void TestOpt::GetArgs(int argc, char *argv[])
{
	int option_index = 0;
	int c = 0;
	while((c = getopt_long(argc, argv, "t:n:", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 't':
				thread_count = std::atoi(optarg);
				break;
			case 'n':
				client_count = std::atoi(optarg);
				break;
			case '?':
				throw StackTraceExcept("Unknown Argument : " + std::string(argv[optind-1]), __STACKINFO__);
			default:
				throw StackTraceExcept("getopt_long gets error(" + std::to_string(c) + ") : " + std::string(argv[optind-1]), __STACKINFO__);
		}
	}

	if(optind < argc)
		addr = std::string(argv[optind++]);

	while(optind < argc)
		throw StackTraceExcept("Unknown Argument : " + std::string(argv[optind++]), __STACKINFO__);
}