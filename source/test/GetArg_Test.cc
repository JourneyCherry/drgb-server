#include <gtest/gtest.h>
#include "GetArg.hpp"

using mylib::utils::GetArg;

class TestOpt : public GetArg
{
	private:
		static struct option long_options[];
	
	public:
		bool single_flag;

		bool str_flag;
		std::string str;

	public:
		void ClearOpt() override
		{
			single_flag = false;
			str_flag = false;
			str = "";
			optind = 1;
		}

		void GetArgs(int argc, char **argv) override
		{
			int option_index = 0;
			int c = 0;
			while((c = getopt_long(argc, argv, "fs:", long_options, &option_index)) != -1)
			{
				switch(c)
				{
					case 'f':
						single_flag = true;
						break;
					case 's':
						str_flag = true;
						str = std::string(optarg);
						break;
					case '?':
						throw std::runtime_error("Unknown Argument");
					default:
						throw std::runtime_error("Error");
				}
			}
			if(optind < argc)
				throw std::runtime_error("Unprocessed Arguments");
		}
};

struct option TestOpt::long_options[] = {
	{"flag", no_argument, 0, 'f'},
	{"str", required_argument, 0, 's'},
	{0, 0, 0, 0}
};

class GetArgTest : public ::testing::Test
{
	protected:
		void Set(std::vector<std::string> &arguments)
		{
			argument.clear();
			argument.push_back((char*)executable.data());
			for(auto &arg : arguments)
				argument.push_back((char*)arg.data());
			//argument.push_back((char*)nullstr.data());
			//argument.push_back(nullptr);
			argv = argument.data();
			argc = argument.size();
		}

	std::string executable = "GetArg_Test";
	std::string nullstr = "\0";
	std::vector<char*> argument;
	char **argv;
	int argc;
};

TEST_F(GetArgTest, ShortArgTest)
{
	std::vector<std::string> args = {
		"-f",
		"-s",
		"Hello World"
	};
	Set(args);
	TestOpt t;
	t.ClearOpt();
	t.GetArgs(argc, argv);
	EXPECT_TRUE(t.single_flag);
	EXPECT_TRUE(t.str_flag);
	EXPECT_STREQ("Hello World", t.str.c_str());

	args = {
		"-s",
		"Hello"
	};
	Set(args);
	t.ClearOpt();
	t.GetArgs(argc, argv);
	EXPECT_FALSE(t.single_flag);
	EXPECT_TRUE(t.str_flag);
	EXPECT_STREQ("Hello", t.str.c_str());

	args = {
		"-f",
		"-u",
		"-s",
		"world"
	};
	Set(args);
	t.ClearOpt();
	EXPECT_ANY_THROW(t.GetArgs(argc, argv));
}

TEST_F(GetArgTest, LongArgTest)
{
	std::vector<std::string> args = {
		"--flag",
		"--str",
		"Hello World"
	};
	Set(args);
	TestOpt t;
	t.ClearOpt();
	t.GetArgs(argc, argv);
	EXPECT_TRUE(t.single_flag);
	EXPECT_TRUE(t.str_flag);
	EXPECT_STREQ("Hello World", t.str.c_str());

	args = {
		"--str",
		"Hello"
	};
	Set(args);
	t.ClearOpt();
	t.GetArgs(argc, argv);
	EXPECT_FALSE(t.single_flag);
	EXPECT_TRUE(t.str_flag);
	EXPECT_STREQ("Hello", t.str.c_str());

	args = {
		"--flag",
		"--str",
		"world",
		"--unknown"
	};
	Set(args);
	t.ClearOpt();
	EXPECT_ANY_THROW(t.GetArgs(argc, argv));
}