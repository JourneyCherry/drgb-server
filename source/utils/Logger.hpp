#pragma once
#include <iostream>
#include <mutex>
#include <syslog.h>
#include <memory>
#include <cstring>
#include <string>
#include <map>
#include <stdexcept>

namespace mylib{
namespace utils{

class Logger
{
	public:
		enum LogType : int
		{
			default_type = -1,
			info = 0,
			auth,
			error,
			debug
		};
	private:
		static std::unique_ptr<Logger> instance_;
		static Logger* instance();
		static std::map<LogType, int> ports;
		const static std::string service_name;
		bool isOpened;
		bool isVerbose;
		std::mutex verbose_mtx;

	public:
		Logger();
		~Logger();
		static void OpenLog(bool = false);
		static void ConfigPort(LogType, int);
		static void log(std::string, LogType=LogType::info);
		static void raise();
		static void raise(std::exception_ptr);
		static std::string strerrno(int);
		static LogType GetType(std::string);
	
	private:
		void OpenLog_(bool);
		void ConfigPort_(LogType, int);
		void log_(std::string, LogType);
		int GetLogPort(int);
};

}
}