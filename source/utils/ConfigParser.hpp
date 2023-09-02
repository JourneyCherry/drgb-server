#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <functional>

namespace mylib{
namespace utils{

/**
 * @brief a global class for Parsing Config file(.conf)
 * 
 */
class ConfigParser
{
	private:
		static std::unordered_map<std::string, std::string> dict;
	public:
		/**
		 * @brief read configuration file and store key-value data.
		 * 
		 * @param path configuration file path
		 * 
		 * @return true if successed.
		 * @return false if file cannot be accessed.
		 */
		static bool ReadFile(const std::string &path);
		/**
		 * @brief Get a string value of 'key'
		 * 
		 * @param key key for search
		 * @param default_value default value if there is no 'key' in configuration.
		 * @return std::string value
		 */
		static std::string GetString(std::string key, std::string default_value = "");
		/**
		 * @brief Get a integer value of 'key'
		 * 
		 * @param key key for search
		 * @param default_value default value if there is no 'key' in configuration or the value isn't integer.
		 * @return int value
		 */
		static int GetInt(std::string key, int default_value = 0);
		/**
		 * @brief Get a double/float value of 'key'
		 * 
		 * @param key key for search
		 * @param default_value default value if there is no 'key' in configuration or the value isn't float.
		 * @return double value
		 */
		static double GetDouble(std::string key, double default_value = 0.0);
	private:
		static bool check_line(const std::string&);
		static void trim(std::string&);
		static void delete_comment(std::string&);
		static std::pair<std::string, std::string> split(const std::string&);
};

}
}