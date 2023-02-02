#include "MyLogger.hpp"

std::unique_ptr<MyLogger> MyLogger::instance_ = nullptr;
std::map<MyLogger::LogType, int> MyLogger::ports;
const std::string MyLogger::service_name = "drgb";
MyLogger* MyLogger::instance()
{
	if(instance_ == nullptr)
		instance_ = std::make_unique<MyLogger>();
	return instance_.get();
}

std::string MyLogger::strerrno(int num)
{
	char *str = std::strerror(num);
	int len = std::strlen(str);
	return std::string(str, len) +  "(" + std::to_string(num) + ")";
}

MyLogger::MyLogger()
{
	static const LogType All[] = {default_type, info, auth, error, debug};
	for(LogType t : All)
		ports.insert({t, -1});
	isOpened = false;
}

MyLogger::~MyLogger()
{
	if(isOpened)
		closelog();
}

void MyLogger::OpenLog(){instance()->OpenLog_();}
void MyLogger::ConfigPort(LogType type, int port){instance()->ConfigPort_(type, port);}
void MyLogger::log(std::string content, LogType type){instance()->log_(content, type);}
void MyLogger::raise()
{
	raise(std::current_exception());
}
void MyLogger::raise(std::exception_ptr e)
{
#ifdef __DEBUG__
	std::rethrow_exception(e);
#else
	//std::exception_ptr로부터 what()을 끌어내는 방법.
	try
	{
		std::rethrow_exception(e);
	}
	catch(const std::exception& ex)
	{
		log(ex.what(), LogType::error);
	}
#endif
}
MyLogger::LogType MyLogger::GetType(std::string name)
{
	if(name == "default")	return default_type;
	if(name == "info")	return info;
	if(name == "auth")	return auth;
	if(name == "error")	return error;
	if(name == "debug") return debug;
	return default_type;
}

void MyLogger::OpenLog_()
{
	if(ports[auth] < 0)
		ports[auth] = LOG_INFO | LOG_AUTH;

	for(std::pair<LogType, int> p : ports)
	{
		if(p.second < 0)
			ConfigPort_(p.first, p.second);
	}
		
	MyLogger::ports[default_type] |= LOG_INFO;

	openlog(service_name.c_str(), LOG_PID, ports[default_type]);
	isOpened = true;
}

void MyLogger::ConfigPort_(LogType type, int port)
{
	int tp = GetLogPort(port);	//Target Port
	if(port <= 0)
		tp = ports[default_type];

	switch(type)
	{
		//case default_type:	break;
		case info:
			tp |= LOG_INFO;
			break;
		case auth:
			tp |= LOG_INFO;
			if(port < 0)
				tp = LOG_INFO | LOG_AUTH;	//별도의 설정 없으면 /var/log/auth.log에 기록.
		case error:
			tp |= LOG_ERR;
			break;
		case debug:
			tp |= LOG_DEBUG;
			break;
	}
	ports[type] = tp;
}

void MyLogger::log_(std::string content, LogType type)
{
	syslog(ports[type], "%s", content.c_str());
}

int MyLogger::GetLogPort(int port)
{
	if(port < 0 || port > 7)
		port = 0;
	return ((port + 16) << 3);	//(16<<3) == LOG_LOCAL0
}