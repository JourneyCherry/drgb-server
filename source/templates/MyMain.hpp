#pragma once
#include <memory>
#include <functional>
#include <pthread.h>
#include "Daemonizer.hpp"
#include "MyServerOpt.hpp"
#include "ConfigParser.hpp"

using mylib::utils::Daemonizer;
using mylib::utils::Create_PidFile;
using mylib::utils::Logger;
using mylib::utils::ConfigParser;

/**
 * @brief Common Process of all Dedicated Servers. It includes reading arguments, opening logger, daemonizing, handling signals, creating pid files, parsing configuration file and start/stop the Server. actual main() function should call 'MyMain::main()'.
 * 
 */
class MyMain
{
	private:
		static void signal_handler(int);
		static std::function<void()> process;
		static std::function<void()> stop;
		std::string service_name;
	public:
		/**
		 * @brief Constructor of MyMain
		 * 
		 * @param name name of the service. It will be used on logger.
		 * @param p actual process of the dedicated server.
		 * @param s stop process of the dedicated server.
		 */
		MyMain(std::string name, std::function<void()> p, std::function<void()> s);
		/**
		 * @brief Common process of all dedicated servers.
		 * 
		 * @param argc the number of arguments
		 * @param argv argument array
		 * @return int exit code
		 */
		int main(int argc, char *argv[]);
};