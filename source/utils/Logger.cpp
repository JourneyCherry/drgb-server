#include "Logger.hpp"

namespace mylib{
namespace utils{

std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::map<Logger::LogType, int> Logger::ports;
const std::string Logger::service_name = "drgb";
Logger* Logger::instance()
{
	if(instance_ == nullptr)
		instance_ = std::make_unique<Logger>();
	return instance_.get();
}

std::string Logger::strerrno(int num)
{
	char *str = std::strerror(num);
	int len = std::strlen(str);
	return std::string(str, len) +  "(" + std::to_string(num) + ")";
}

Logger::Logger()
{
	static const LogType All[] = {default_type, info, auth, error, debug};
	for(LogType t : All)
		ports.insert({t, -1});
	isOpened = false;
}

Logger::~Logger()
{
	if(isOpened)
		closelog();
}

void Logger::OpenLog(bool _verbose){instance()->OpenLog_(_verbose);}
void Logger::ConfigPort(LogType type, int port){instance()->ConfigPort_(type, port);}
void Logger::log(std::string content, LogType type){instance()->log_(content, type);}
void Logger::raise()
{
	raise(std::current_exception());
}
void Logger::raise(std::exception_ptr e)
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
Logger::LogType Logger::GetType(std::string name)
{
	if(name == "default")	return default_type;
	if(name == "info")	return info;
	if(name == "auth")	return auth;
	if(name == "error")	return error;
	if(name == "debug") return debug;
	return default_type;
}

void Logger::OpenLog_(bool _verbose)
{
	isVerbose = _verbose;

	if(ports[auth] < 0)
		ports[auth] = LOG_INFO | LOG_AUTH;

	for(std::pair<LogType, int> p : ports)
	{
		if(p.second < 0)
			ConfigPort_(p.first, p.second);
	}
		
	Logger::ports[default_type] |= LOG_INFO;

	openlog(service_name.c_str(), LOG_PID, ports[default_type]);
	
	isOpened = true;
}

void Logger::ConfigPort_(LogType type, int port)
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

void Logger::log_(std::string content, LogType type)
{
	if(isVerbose)
	{
		static std::map<LogType, std::string> type2str = {
			{LogType::info, "INFO"},
			{LogType::auth, "AUTH"},
			{LogType::error, "ERROR"},
			{LogType::debug, "DEBUG"},
			{LogType::default_type, "DEFAULT"}
		};
		std::unique_lock<std::mutex> lk(verbose_mtx);
		std::cout << "[" << type2str[type] << "] " << content << std::endl;
	}
	syslog(ports[type], "%s", content.c_str());
}

int Logger::GetLogPort(int port)
{
	if(port < 0 || port > 7)
		port = 0;
	return ((port + 16) << 3);	//(16<<3) == LOG_LOCAL0
}

}
}