#include <gtest/gtest.h>
#include <hiredis/hiredis.h>
#include <thread>
#include <chrono>
#include "MyRedis.hpp"

void ShowReply(std::string name, MyRedisReply reply, int indent)
{
	if(reply.type == 0)
	{
		std::cout << name << " gets nullptr" << std::endl;
		return;
	}
	indent = std::max(0, indent);
	std::string indent_str = "";
	for(int i = 0;i<indent + 1;i++)
		indent_str += "  ";
	switch(reply.type)
	{
		case REDIS_REPLY_STRING:
			std::cout << name << "(string) -> " << reply.str << std::endl;
			break;
		case REDIS_REPLY_ARRAY:
			std::cout << name << "(array)" << std::endl;
			for(int i = 0;i<reply.elements.size();i++)
				ShowReply(indent_str + std::to_string(i+1) + ")", reply.elements[i], indent+1);
			break;
		case REDIS_REPLY_INTEGER:
			std::cout << name << "(integer) -> " << std::to_string(reply.integer) << std::endl;
			break;
		case REDIS_REPLY_NIL:
			std::cout << name << "(nil)" << std::endl;
			break;
		case REDIS_REPLY_STATUS:
			std::cout << name << "(status) -> " << reply.str << std::endl;
			break;
		case REDIS_REPLY_ERROR:
			std::cout << name << "(error) -> " << reply.str << std::endl;
			break;
		case REDIS_REPLY_DOUBLE:
			std::cout << name << "(double) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_BOOL:
			std::cout << name << "(bool) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_MAP:
			std::cout << name << "(map) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_SET:
			std::cout << name << "(set) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_ATTR:
			std::cout << name << "(attr) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_PUSH:
			std::cout << name << "(push) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_BIGNUM:
			std::cout << name << "(BIGNUM) value not implemented" << std::endl;
			break;
		case REDIS_REPLY_VERB:
			std::cout << name << "(verb) value not implemented" << std::endl;
			break;
		default:
			std::cout << name << "(unknown type " << std::to_string(reply.type) << ")" << " value not implemented" << std::endl;
			break;
	}
}

class MyRedisTest : public MyRedis
{
	private:
		static constexpr int sessionTimeout = 10000;
	public:
		MyRedisTest() : MyRedis(sessionTimeout){}
		std::vector<MyRedisReply> TestQuery(std::initializer_list<std::string> queries)
		{
			return Query(queries);
		}
		std::vector<MyRedisReply> TestQuery(std::vector<std::string> queries)
		{
			return Query(queries);
		}
		ErrorCode GetEC()
		{
			return GetErrorCode();
		}
};

class RedisTestFixture : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			ErrorCode ec = redis.Connect("localhost", 6379);
			ASSERT_TRUE(ec.isSuccessed()) << "Connect() : " << ec.message_code();
		}
		void TearDown() override
		{
			redis.Close();
		}

	public:
		MyRedisTest redis;
};

TEST_F(RedisTestFixture, ConnectionTest)
{
	std::string command = "SET myTest \'Hello World!\' PX 1000";
	auto replies = redis.TestQuery({command});
	ASSERT_GT(replies.size(), 0) << command << " -> " << redis.GetEC().message_code();
	ShowReply(command, replies[0], 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	command = "GET myTest";
	replies = redis.TestQuery({command});
	ASSERT_GT(replies.size(), 0) << command << " -> " << redis.GetEC().message_code();
	ShowReply(command, replies[0], 0);
}

TEST_F(RedisTestFixture, AtomicTest)
{
	std::vector<std::string> commands = {
		"WATCH a",
		"WATCH b",
		"MULTI",
		"SET a hello PX 150000",
		"SET b world PX 150000",
		"EXEC",
		"GET a",
		"GET b",
		"DEL a b"
	};

	auto replies = redis.TestQuery(commands);
	ASSERT_GT(replies.size(), 0) << "Query Failed : " << redis.GetEC().message_code();

	for(int i = 0;i<commands.size();i++)
		ShowReply(commands[i], replies[i], 0);
}

TEST_F(RedisTestFixture, QueryTest)
{
	unsigned long long id = 5;
	std::string idstr = std::to_string(id);
	Hash_t cookie({'a', 'b', 'c', 'd', 'e', 'f', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	std::string cookieStr = "abcdef";
	std::string serverName = "match";
	std::string nextServerName = "battle";

	//Set Information
	auto ec = redis.SetInfo(id, cookie, serverName);
	ASSERT_TRUE(ec.isSuccessed()) << "SetInfo() : " << ec.message_code();

	//Get Information from both way
	auto resultFromID = redis.GetInfoFromID(id);
	ASSERT_TRUE(resultFromID.isSuccessed()) << "GetInfoFromID() : " << resultFromID.error().message_code();
	EXPECT_EQ(resultFromID->first, cookie);
	EXPECT_STREQ(resultFromID->second.c_str(), serverName.c_str());

	auto resultFromCookie = redis.GetInfoFromCookie(cookie);
	ASSERT_TRUE(resultFromCookie.isSuccessed()) << "GetInfoFromCookie() : " << resultFromCookie.error().message_code();
	EXPECT_EQ(resultFromCookie->first, id);
	EXPECT_STREQ(resultFromCookie->second.c_str(), serverName.c_str());

	//Set New Information
	ec = redis.SetInfo(id, cookie, nextServerName);
	ASSERT_TRUE(ec.isSuccessed()) << "SetInfo() : " << ec.message_code();

	resultFromID = redis.GetInfoFromID(id);
	ASSERT_TRUE(resultFromID.isSuccessed()) << "GetInfoFromID() : " << resultFromID.error().message_code();
	EXPECT_EQ(resultFromID->first, cookie);
	EXPECT_STREQ(resultFromID->second.c_str(), nextServerName.c_str());

	//Remove Wrong Information
	EXPECT_FALSE(redis.ClearInfo(id + 1, cookie, serverName)) << "Clear Wrong Info Successed";
	EXPECT_FALSE(redis.ClearInfo(id, {0, 0, 0, 0, 0, 0}, serverName)) << "Clear Wrong Info Successed";
	EXPECT_FALSE(redis.ClearInfo(id, cookie, serverName)) << "Clear Wrong Info Successed";

	//Remove Information
	EXPECT_TRUE(redis.ClearInfo(id, cookie, nextServerName)) << "Clear Info Failed";

	//Get Removed Information
	resultFromID = redis.GetInfoFromID(id);
	ASSERT_FALSE(resultFromID.isSuccessed());
	ASSERT_EQ(resultFromID.error().code(), ERR_NO_MATCH_ACCOUNT);

	resultFromCookie = redis.GetInfoFromCookie(cookie);
	ASSERT_FALSE(resultFromCookie.isSuccessed());
	ASSERT_EQ(resultFromCookie.error().code(), ERR_NO_MATCH_ACCOUNT);
}