#pragma once
#include <memory>
#include <functional>
#include "MyDaemoner.hpp"
#include "MyGetOpt.hpp"
#include "MyLogger.hpp"
#include "MyConfigParser.hpp"

class MyMain
{
	private:
		static void signal_handler(int);
		static std::function<void()> process;
		static std::function<void()> stop;
	public:
		MyMain(std::function<void()>, std::function<void()>);
		int main(int, char*[]);
};