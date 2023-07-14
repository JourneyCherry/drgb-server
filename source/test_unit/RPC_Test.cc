#include <gtest/gtest.h>
#include <thread>
#include <memory>
#include "ManagerServiceServer.hpp"
#include "ManagerServiceClient.hpp"
#include "MatchServiceServer.hpp"
#include "BattleServiceClient.hpp"

using grpc::Server;
using grpc::ServerBuilder;

class RPCTestFixture : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			grpc::EnableDefaultHealthCheckService(true);
			Builder.AddListeningPort("0.0.0.0:52431", grpc::InsecureServerCredentials());
			Builder.RegisterService(&mgrServer);
			Builder.RegisterService(&match);
			ServiceServer = Builder.BuildAndStart();
			ServiceThread = std::thread([this]()
			{
				this->ServiceServer->Wait();
			});
		}

		void TearDown() override
		{
			ServiceServer->Shutdown();
			if(ServiceThread.joinable())
				ServiceThread.join();
		}

		virtual size_t GetUsage() { return 0; }
		virtual bool CheckAccount(Account_ID_t) { return false; }
		virtual size_t GetClientUsage() { return 0; }
		virtual std::map<std::string, size_t> GetConnectUsage() 
		{
			std::map<std::string, size_t> connected;
			connected.insert({"default", 1});
			return connected;
		}
	private:
		std::thread ServiceThread;
		std::unique_ptr<Server> ServiceServer;
		ServerBuilder Builder;

	public:
		RPCTestFixture() : 
			port(52431),
			mgrServer(
				std::bind(&RPCTestFixture::GetUsage, this),
				std::bind(&RPCTestFixture::CheckAccount, this, std::placeholders::_1),
				std::bind(&RPCTestFixture::GetClientUsage, this),
				std::bind(&RPCTestFixture::GetConnectUsage, this)
			) {}

		int port;

		MatchServiceServer match;
		ManagerServiceServer mgrServer;
};

TEST_F(RPCTestFixture, M2BTest)
{
	Seed_t machine_id = 1;
	BattleServiceClient battle("0.0.0.0", port, machine_id);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));	//Wait for Connection
	ASSERT_GT(match.GetUsage(), 0);
	Account_ID_t lp = 5, rp = 7;
	battle.SetCallback([lp, rp](Account_ID_t l, Account_ID_t r)
	{
		EXPECT_EQ(l, lp);
		EXPECT_EQ(r, rp);
	});
	Seed_t battle_id = match.TransferMatch(lp, rp);
	EXPECT_EQ(battle_id, machine_id);
}

TEST_F(RPCTestFixture, DuplicateTest)
{
	Account_ID_t lp = 5, rp = 7;
	std::vector<std::shared_ptr<BattleServiceClient>> vec;
	for(int i = 1;i<=5;i++)
	{
		auto client = std::make_shared<BattleServiceClient>("0.0.0.0", port, i, nullptr);
		client->SetCallback([lp, rp](Account_ID_t l, Account_ID_t r)
		{
			EXPECT_EQ(l, lp);
			EXPECT_EQ(r, rp);
		});
		vec.push_back(client);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));	//Wait for Connection
	ASSERT_EQ(match.GetUsage(), 5);
	vec[0]->SetUsage(5);
	vec[1]->SetUsage(4);
	vec[2]->SetUsage(3);
	vec[3]->SetUsage(4);
	vec[4]->SetUsage(5);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));	//Wait for Connection
	auto target_id = match.TransferMatch(lp, rp);
	EXPECT_EQ(target_id, 3);

	BattleServiceClient dup("0.0.0.0", port, 3);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));	//Wait for Connection
	ASSERT_FALSE(dup.is_open());
	ASSERT_EQ(match.GetUsage(), 5);
}

class ManagerTestFixture : public RPCTestFixture
{
	protected:
		size_t GetUsage() override { return 5; }
		bool CheckAccount(Account_ID_t id) override { return (id == 5); }
		size_t GetClientUsage() override { return 9; }
		std::map<std::string, size_t> GetConnectUsage() override
		{
			std::map<std::string, size_t> connected = RPCTestFixture::GetConnectUsage();
			connected.insert({"First", 1});
			connected.insert({"Second", 2});
			connected.insert({"Third", 3});
			return connected;
		}
};

TEST_F(ManagerTestFixture, ManagerTest)
{
	ManagerServiceClient mgr("0.0.0.0", port);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));	//Wait for Connection

	auto usage = mgr.GetUsage();
	ASSERT_TRUE(usage.isSuccessed());
	EXPECT_EQ(usage.value(), 5);

	auto account = mgr.CheckAccount(5);
	ASSERT_TRUE(account.isSuccessed());
	EXPECT_TRUE(account.value());

	account = mgr.CheckAccount(1);
	ASSERT_TRUE(account.isSuccessed());
	EXPECT_FALSE(account.value());

	auto client = mgr.GetClientUsage();
	ASSERT_TRUE(client.isSuccessed());
	auto web = *client;
	EXPECT_EQ(web, 9);

	auto connect = mgr.GetConnectUsage();
	ASSERT_TRUE(connect.isSuccessed());

	std::map<std::string, size_t> answered({{"default", 1}, {"First", 1}, {"Second", 2}, {"Third", 3}});
	ASSERT_EQ(answered.size(), connect->size());
	for(auto &[key, value] : answered)
	{
		auto iter = connect->find(key);
		ASSERT_NE(iter, connect->end());
		EXPECT_STREQ(iter->first.c_str(), key.c_str());
		EXPECT_EQ(iter->second, value);
	}
}