#pragma once
#include "GetArg.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::GetArg;
using mylib::utils::StackTraceExcept;

class TestOpt : public GetArg
{
	private:
		static struct option long_options[];
	public:
		int thread_count;
		int client_count;
		std::string addr;

	public:
		TestOpt();
		void ClearOpt() override;
		void GetArgs(int, char**) override;
};