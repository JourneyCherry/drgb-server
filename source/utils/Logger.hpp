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

/**
 * @brief Logging Class using syslog-ng.(Singleton Pattern). Every log will be through Local 0~7 facilities, So, each local facilities must be set in syslog-ng's configuration files first.
 * 
 */
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
		std::string service_name;
		bool isOpened;
		bool isVerbose;
		std::mutex verbose_mtx;

	public:
		Logger();
		~Logger();
		/**
		 * @brief Start logging. 'ConfigPort()' must be done before open.
		 * 
		 * @param name Name of the Service to log.
		 * @param _verbose whether logging through standard output or not(syslog-ng).
		 */
		static void OpenLog(std::string name, bool _verbose = false);
		/**
		 * @brief Assign local facility to log type.
		 * 
		 * @param type type of log
		 * @param port local facility number.(0 ~ 7)
		 */
		static void ConfigPort(LogType type, int port);
		/**
		 * @brief Log message as 'type'
		 * 
		 * @param content string message to log
		 * @param type log type.
		 */
		static void log(std::string content, LogType type = LogType::info);
		/**
		 * @brief Get error message according to errno.
		 * 
		 * @param num 
		 * @return std::string 
		 */
		static std::string strerrno(int num);
		/**
		 * @brief Get Log type by string name.
		 * 
		 * @param name type name
		 * @return LogType type enum.
		 */
		static LogType GetType(std::string name);
	
	private:
		void OpenLog_(std::string, bool);
		void ConfigPort_(LogType, int);
		void log_(std::string, LogType);
		int GetLogPort(int);
};

}
}