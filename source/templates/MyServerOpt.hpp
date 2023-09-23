#pragma once
#include "GetArg.hpp"
#include "Logger.hpp"

using mylib::utils::GetArg;
using mylib::utils::Logger;
using mylib::utils::StackTraceExcept;

/**
 * @brief Executable's Arguments Parser for Server
 * 
 */
class MyServerOpt : public GetArg
{
	private:
		static struct option long_options[];
	public:
		bool daemon_flag;

		bool pid_flag;
		std::string pid_path;
		
		std::string conf_path;

		bool verbose_flag;
	public:
		/**
		 * @brief Constructor of MyServerOpt.
		 * 
		 * @param argc the number of arguments
		 * @param argv argument array
		 */
		MyServerOpt(int argc, char** argv);
		/**
		 * @brief Clear and initialize all options set by arguments.
		 * 
		 */
		void ClearOpt() override;
		/**
		 * @brief Parse Arguments. **Do Not Call this method because it will be called in constructor automatically.**
		 * 
		 * @param argc 
		 * @param argv 
		 */
		void GetArgs(int argc, char** argv) override;
};