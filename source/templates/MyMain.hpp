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