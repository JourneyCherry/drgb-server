#pragma once
#include <queue>
#include "GetArg.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::GetArg;
using mylib::utils::StackTraceExcept;

class MgrOpt : public GetArg
{
	private:
		static struct option long_options[];
	public:
		std::string addr;
		int port;
		bool shell_mode;
		std::queue<std::string> commands;

	public:
		MgrOpt();
		void ClearOpt() override;
		void GetArgs(int, char**) override;
};