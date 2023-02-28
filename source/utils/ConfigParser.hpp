#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>

namespace mylib{
namespace utils{

class ConfigParser
{
	private:
		static std::unordered_map<std::string, std::string> dict;
	public:
		static bool ReadFile(const std::string&);
		static std::string GetString(std::string, std::string="");
		static int GetInt(std::string, int=0);
		static double GetDouble(std::string, double=0.0);
	private:
		static bool check_line(const std::string&);
		static void trim(std::string&);
		static void delete_comment(std::string&);
		static std::pair<std::string, std::string> split(const std::string&);
};

}
}