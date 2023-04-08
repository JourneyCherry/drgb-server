#include "ConfigParser.hpp"

namespace mylib{
namespace utils{

std::unordered_map<std::string, std::string> ConfigParser::dict = std::unordered_map<std::string, std::string>();

bool ConfigParser::ReadFile(const std::string &path)
{
	std::ifstream openFile;
	openFile.open(path);
	if(!openFile.is_open())
		return false;
	std::string line;
	while(std::getline(openFile, line))
	{
		delete_comment(std::ref(line));
		trim(std::ref(line));
		if(!check_line(line))
			continue;
		auto [key, value] = split(line);
		if(key.length() <= 0)
			continue;
		dict.insert({key, value});
	}
	return true;
}

std::string ConfigParser::GetString(std::string key, std::string default_value)
{
	if(dict.find(key) == dict.end())
		return default_value;
	return dict[key];
}

int ConfigParser::GetInt(std::string key, int default_value)
{
	int result;
	try
	{
		if(dict.find(key) == dict.end())
			return default_value;
		result = std::stoi(dict[key]);
	}
	catch(const std::exception& e)
	{
		return default_value;
	}
	return result;
}

double ConfigParser::GetDouble(std::string key, double default_value)
{
	double result;
	try
	{
		if(dict.find(key) == dict.end())
			return default_value;
		result = std::stod(dict[key]);
	}
	catch(const std::exception& e)
	{
		return default_value;
	}
	return result;
}

bool ConfigParser::check_line(const std::string &line)
{
	int max = line.length();
	char quotes = '\0';
	int num_of_equal = 0;
	for(int i = 0;i<max&&num_of_equal<2;i++)
	{
		if(quotes != '\0')
		{
			quotes = (quotes==line[i])?'\0':quotes;
			continue;
		}
		if(line[i] == '\"' || line[i] == '\'')
		{
			quotes = line[i];
			continue;
		}
		if(line[i] == '=')
		{
			num_of_equal++;
			continue;
		}
	}
	return num_of_equal == 1 && quotes == '\0';	//따옴표는 닫혀 있어야 한다.
}

void ConfigParser::trim(std::string &src)
{
	static std::string trimmer = " \t\n\r\f\v";
	//erase(0, std::string::npos) == erase(std::string::npos + 1) == erase(0) == clear()
	src.erase(src.find_last_not_of(trimmer) + 1);
	src.erase(0, src.find_first_not_of(trimmer));
}

void ConfigParser::delete_comment(std::string &src)
{
	size_t pos = src.find_first_of('#');
	if(pos == std::string::npos)
		return;
	src.erase(src.find_first_of('#'));
}

std::pair<std::string, std::string> ConfigParser::split(const std::string &src)
{
	std::string front = src;
	std::string end = src;
	int equal = src.find_first_of("=");	//TODO : 따옴표 안에 있는 '='문자는 예외로 해야 함.
	if(equal == std::string::npos)
		return {"", src};
	front.erase(equal);
	end.erase(0, equal+1);
	trim(front);
	trim(end);
	if(end.find_first_of("\"\'") != std::string::npos)
	{
		end.erase(0, end.find_first_of("\"\'") + 1);
		end.erase(end.find_first_of("\'\""));
	}
	return {front, end};
}

}
}