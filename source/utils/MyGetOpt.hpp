#pragma once
#include <getopt.h>
#include <string>
#include <functional>
#include "MyExcepts.hpp"

namespace MyCommon
{
	class MyGetOpt
	{
		private:
			static struct option long_options[];

			bool splitarg(std::string, std::string&, std::string&);
		public:
			bool daemon_flag;

			bool pid_flag;
			std::string pid_path;
			
			std::string conf_path;
		public:
			MyGetOpt();
			MyGetOpt(int, char**);
			void GetOpt(int, char**);
	};
}