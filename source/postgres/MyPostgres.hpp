#pragma once
#include <string>
#include <pqxx/pqxx>
#include <memory>
#include <thread>
#include <chrono>
#include "ConfigParser.hpp"
#include "StackTraceExcept.hpp"

using mylib::utils::ConfigParser;
using mylib::utils::StackTraceExcept;
using mylib::utils::Logger;

class MyPostgres
{
	private:
		static bool isRunning;

		static std::unique_ptr<pqxx::connection> db_c;
		std::unique_ptr<pqxx::work> db_w;
	public:
		static void Open();
		static void Close();
		static bool isOpened();
	public:
		MyPostgres();
		MyPostgres(MyPostgres&&);
		MyPostgres& operator=(MyPostgres&&);
		~MyPostgres();
		void abort();
		void commit();
		std::string quote(std::string);
		std::string quote_name(std::string);
		std::string quote_raw(const unsigned char*, size_t);
		pqxx::result exec(const std::string& query);
		pqxx::row exec1(const std::string& query);
};