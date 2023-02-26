#pragma once
#include <memory>
#include <functional>
#include <pthread.h>
#include "MyDaemoner.hpp"
#include "MyServerOpt.hpp"
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