#include <gtest/gtest.h>
#include <filesystem>
#include <unistd.h>
#include "Daemonizer.hpp"

using namespace mylib::utils;

TEST(DaemonizerTest, DaemonizeTest)
{
	bool result = Daemonizer();
	pid_t ppid = getppid();
	if(ppid == 0)
		EXPECT_TRUE(result);
	else
		EXPECT_FALSE(result) << "ppid : " << ppid;
}

class FileTest : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			filepath = ::testing::TempDir() + "Daemonizer_Test.pid";
		}

		void TearDown() override
		{
			std::filesystem::remove(filepath);
		}

	std::string filepath;
};

TEST_F(FileTest, PidFileTest)
{
	ASSERT_NO_THROW(Create_PidFile(filepath));
	EXPECT_EQ(0, ::access(filepath.c_str(), F_OK));
}