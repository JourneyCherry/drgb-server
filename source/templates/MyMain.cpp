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

#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "Main");
#endif

	try
	{
		MyServerOpt opts(argc, argv);
		Logger::OpenLog(opts.verbose_flag);
		if(opts.daemon_flag)
		{
			if(!Daemonizer())
				return 0;
		}
		else
		{
			signal(SIGINT, MyMain::signal_handler);
		}
		signal(SIGTERM, signal_handler);
		signal(SIGPIPE, SIG_IGN);
		if(opts.pid_flag)
			Create_PidFile(opts.pid_path);
		if(!ConfigParser::ReadFile(opts.conf_path))
			throw std::runtime_error("Cannot Open Config File");

		process();
	}
	catch(StackTraceExcept e)
	{
		e.stack(__STACKINFO__);
		Logger::log(e.what());
		exit_code = -1;
	}
	catch(const std::exception &e)
	{
		Logger::log("[Standard Exception] : " + std::string(e.what()), Logger::error);
		exit_code = -1;
	}

	stop();
	return exit_code;
}