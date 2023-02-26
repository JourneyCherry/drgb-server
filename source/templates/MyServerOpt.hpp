#pragma once
#include "MyGetOpt.hpp"

class MyServerOpt : public MyCommon::MyGetOpt
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
		void GetOpt(int, char**) override;
};