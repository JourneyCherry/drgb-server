#include "MyMain.hpp"

std::function<void()> MyMain::process = nullptr;
std::function<void()> MyMain::stop = nullptr;

void MyMain::signal_handler(int signal_code)
{
	if(signal_code == SIGTERM || signal_code == SIGINT)
	{
		MyMain::stop();
		signal(signal_code, SIG_DFL);
	}
}

MyMain::MyMain(std::function<void()> p, std::function<void()> s)
{
	process = p;
	stop = s;
}

int MyMain::main(int argc, char *argv[])
{
	int exit_code = 0;
	try
	{
		MyServerOpt opts(argc, argv);
		MyLogger::OpenLog();
		if(opts.daemon_flag)
		{
			if(!MyCommon::Daemonizer())
				return 0;
		}
		else
		{
			signal(SIGINT, MyMain::signal_handler);
		}
		signal(SIGTERM, signal_handler);
		signal(SIGPIPE, SIG_IGN);
		if(opts.pid_flag)
			MyCommon::Create_PidFile(opts.pid_path);
		if(!MyConfigParser::ReadFile(opts.conf_path))
			throw std::runtime_error("Cannot Open Config File");

		process();
	}
	catch(MyExcepts e)
	{
		e.stack(__STACKINFO__);
		MyLogger::log(e.what());
		exit_code = -1;
	}
	catch(const std::exception &e)
	{
		MyLogger::log("[Standard Exception] : " + std::string(e.what()), MyLogger::error);
		exit_code = -1;
	}

	stop();
	return exit_code;
}