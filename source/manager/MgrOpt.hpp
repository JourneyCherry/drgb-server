#pragma once
#include <queue>
#include "GetArg.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::GetArg;
using mylib::utils::StackTraceExcept;

/**
 * @brief Executable's Arguments Parser for Manager
 * 
 */
class MgrOpt : public GetArg
{
	private:
		static struct option long_options[];
	public:
		std::string addr;	//Server Address to manage(default 'localhost')
		int port;			//Server Port to manage(default '52431')
		bool shell_mode;	//Shell Mode.(default false) If it's true, CLI will appear.
		std::queue<std::string> commands;	//Command Lists typed by Executable's Arguments.(default Empty)

	public:
		/**
		 * @brief Constructor of Arguments Parser for Manager. You must call 'GetArgs()'.
		 * 
		 */
		MgrOpt();
		/**
		 * @brief Clear and initialize all options set by arguments.
		 * 
		 */
		void ClearOpt() override;
		/**
		 * @brief Parse Arguments.
		 * 
		 * @param argc 
		 * @param argv 
		 */
		void GetArgs(int argc, char **argv) override;
};