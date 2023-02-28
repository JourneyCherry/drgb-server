#pragma once
#include "GetArg.hpp"

using mylib::utils::GetArg;
using mylib::utils::Logger;
using mylib::utils::StackTraceExcept;

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
		MyServerOpt(int argc, char** argv);
		void ClearOpt() override;
		void GetArgs(int, char**) override;
};