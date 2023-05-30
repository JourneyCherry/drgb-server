#include <gtest/gtest.h>
#include <boost/asio/thread_pool.hpp>
#include <atomic>
#include "MyConnectee.hpp"
#include "MyConnectorPool.hpp"

#define THREAD_NUM 1

class ConnectorTestFixture : public ::testing::Test
{
	protected:
		void SetUp() override
		{
		}

		void TearDown() override
		{
			tor.Close();
			tee.Close();
			EXPECT_EQ(target_tee.load(), count_tee.load());
			EXPECT_EQ(target_tor.load(), count_tor.load());
		}

	public:
		ConnectorTestFixture() : teeword("tee"), torword("tor"), tee(teeword, 0, THREAD_NUM, nullptr), tor(torword, THREAD_NUM, nullptr), target_tee(0), target_tor(0), count_tee(0), count_tor(0)
		{
			port = tee.GetPort();			
		}
		int port;
		std::string teeword;
		std::string torword;

		MyConnectee tee;
		MyConnectorPool tor;

		std::atomic<int> target_tee;
		std::atomic<int> target_tor;
		std::atomic<int> count_tee;
		std::atomic<int> count_tor;
};

TEST_F(ConnectorTestFixture, SingleConnectorTest)
{
	target_tee = 2;
	target_tor = 2;
	auto inquiry = [](ByteQueue request) -> ByteQueue
	{
		byte header = request.pop<byte>();
		byte answer = ERR_PROTOCOL_VIOLATION;
		if(header == REQ_STARTMATCH)
			answer = SUCCESS;
		return ByteQueue::Create<byte>(answer);
	};

	tee.Accept(torword, inquiry);
	tor.Connect("localhost", port, teeword, inquiry);

	std::chrono::system_clock::duration timeout_time = std::chrono::milliseconds(5000);
	std::chrono::system_clock::time_point timer_start = std::chrono::system_clock::now();

	while(tor.GetAuthorized() <= 0 || tee.GetAuthorized() <= 0)	//TODO : KeyExchange, Authentication까지 끝날때까지 기다려야한다.
	{
		auto dur = std::chrono::system_clock::now() - timer_start;
		if(dur.count() > timeout_time.count())
		{
			EXPECT_TRUE(false) << "Connection Timeout!";
			return;
		}
	}

	tee.Request(torword, ByteQueue::Create<byte>(REQ_STARTMATCH), [this](std::shared_ptr<MyConnector> connector, Expected<ByteQueue, StackErrorCode> answer)
	{
		this->count_tee.fetch_add(1);
		if(!answer)
		{
			EXPECT_TRUE(answer.isSuccessed()) << answer.error().message_code();
			return;
		}
		byte header = answer->pop<byte>();
		EXPECT_EQ(header, SUCCESS) << "connectee -> connector : " << (int)header;
	});

	tee.Request(torword, ByteQueue::Create<byte>(REQ_PAUSEMATCH), [this](std::shared_ptr<MyConnector> connector, Expected<ByteQueue, StackErrorCode> answer)
	{
		this->count_tee.fetch_add(1);
		if(!answer)
		{
			EXPECT_TRUE(answer.isSuccessed()) << answer.error().message_code();
			return;
		}
		byte header = answer->pop<byte>();
		EXPECT_NE(header, SUCCESS) << "connectee -> connector : " << (int)header;
	});

	{
		auto answer = tor.Request(teeword, ByteQueue::Create<byte>(REQ_STARTMATCH));
		count_tor.fetch_add(1);
		if(!answer)
		{
			EXPECT_TRUE(answer.isSuccessed()) << "connector -> connectee : " << answer.error().message_code();
		}
		else
		{
			byte header = answer->pop<byte>();
			EXPECT_EQ(header, SUCCESS) << "connector -> connectee : " << (int)header;
		}
	}

	{
		auto answer = tor.Request(teeword, ByteQueue::Create<byte>(REQ_PAUSEMATCH));
		count_tor.fetch_add(1);
		if(!answer)
		{
			EXPECT_TRUE(answer.isSuccessed()) << "connector -> connectee : " << answer.error().message_code();
		}
		else
		{
			byte header = answer->pop<byte>();
			EXPECT_NE(header, SUCCESS) << "connector -> connectee : " << (int)header;
		}
	}
}

class Host
{
	private:
		std::string keyword;
		MyConnectee connectee;
		MyConnectorPool connector;

		ByteQueue Inquiry(ByteQueue request)
		{
			byte header = request.pop<byte>();
			switch(header)
			{
				case REQ_STARTMATCH:
					return ByteQueue::Create<byte>(ANS_MATCHMADE);
			}
			return ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
		}
	public:
		Host(std::string name, int port, int thread) : keyword(name), connectee(keyword, port, thread, nullptr), connector(keyword, thread, nullptr) {}
		~Host()
		{
			connectee.Close();
			connector.Close();
		}

		void ReadyFor(std::string remote_keyword)
		{
			connectee.Accept(remote_keyword, std::bind(&Host::Inquiry, this, std::placeholders::_1));
		}

		void AccessTo(std::string addr, int port, std::string remote_keyword)
		{
			connector.Connect(addr, port, remote_keyword, std::bind(&Host::Inquiry, this, std::placeholders::_1));
		}

		bool Wait(int count, int ms)
		{
			std::chrono::system_clock::duration timeout_time = std::chrono::milliseconds(ms);
			std::chrono::system_clock::time_point timer_start = std::chrono::system_clock::now();

			while(connector.GetAuthorized() < count || connectee.GetAuthorized() < count)	//KeyExchange, Authentication까지 끝날때까지 기다려야한다.
			{
				auto dur = std::chrono::system_clock::now() - timer_start;
				if(dur.count() > timeout_time.count())
				{
					EXPECT_TRUE(false) << "Connection Timeout!";
					return false;
				}
			}
			return true;
		}

		ByteQueue Requestee(std::string remote_keyword, ByteQueue request)
		{
			std::promise<void> promise;
			std::future<void> future = promise.get_future();
			ByteQueue answer;
			connectee.Request(remote_keyword, request, [&](std::shared_ptr<MyConnector> tor, Expected<ByteQueue, StackErrorCode> result)
			{
				if(result)
					answer = *result;
				promise.set_value();
			});
			future.get();
			return answer;
		}

		ByteQueue Requestor(std::string remote_keyword, ByteQueue request)
		{
			auto result = connector.Request(remote_keyword, request);
			if(!result)
				return ByteQueue();
			else
				return *result;
		}

		int GetPort() const
		{
			return connectee.GetPort();
		}

		std::string GetKeyword() const
		{
			return keyword;
		}
};

TEST(ConnectorTest, CircularTest)
{
	constexpr int host_num = 3;
	int each_thread_num = std::thread::hardware_concurrency() / host_num;
	std::vector<std::shared_ptr<Host>> hosts;

	for(int i = 0;i<host_num;i++)
	{
		std::string name = "host" + std::to_string(i);
		hosts.push_back(std::make_shared<Host>(name, 0, each_thread_num));
	}

	for(int i = 0;i<host_num;i++)
	{
		int prev = (i-1<0)?host_num-1:i-1;
		int next = (i+1)%host_num;

		hosts[i]->ReadyFor(hosts[prev]->GetKeyword());
		hosts[i]->AccessTo("localhost", hosts[next]->GetPort(), hosts[next]->GetKeyword());
	}

	for(int i = 0;i<host_num;i++)
	{
		if(!hosts[i]->Wait(1, 5000))	//For Reconnection
			return;
	}

	for(int i = 0;i<host_num;i++)
	{
		int prev = (i-1<0)?host_num-1:i-1;
		int next = (i+1)%host_num;

		std::string prev_key = hosts[prev]->GetKeyword();
		std::string next_key = hosts[next]->GetKeyword();

		ByteQueue succ_request = ByteQueue::Create<byte>(REQ_STARTMATCH);
		ByteQueue fail_request = ByteQueue::Create<byte>(REQ_LOGIN);

		{
			auto succ_result = hosts[i]->Requestee(prev_key, succ_request);
			EXPECT_GE(succ_result.Size(), 1);
			if(succ_result.Size() > 0)
				EXPECT_EQ(succ_result.pop<byte>(), ANS_MATCHMADE);

			auto fail_result = hosts[i]->Requestee(prev_key, fail_request);
			EXPECT_GE(fail_result.Size(), 1);
			if(fail_result.Size() > 0)
				EXPECT_EQ(fail_result.pop<byte>(), ERR_PROTOCOL_VIOLATION);
		}

		{
			auto succ_result = hosts[i]->Requestor(next_key, succ_request);
			EXPECT_GE(succ_result.Size(), 1);
			if(succ_result.Size() > 0)
				EXPECT_EQ(succ_result.pop<byte>(), ANS_MATCHMADE);

			auto fail_result = hosts[i]->Requestor(next_key, fail_request);
			EXPECT_GE(fail_result.Size(), 1);
			if(fail_result.Size() > 0)
				EXPECT_EQ(fail_result.pop<byte>(), ERR_PROTOCOL_VIOLATION);
		}
	}
}

TEST(ConnectorTest, OctopusTest)
{
	constexpr int key_num = 10;
	constexpr int fal_num = 10;
	MyConnectee connectee("server", 0, 0, nullptr);

	std::vector<std::shared_ptr<MyConnectorPool>> connectors;
	boost::asio::io_context ioc;

	auto inquiry = [](int id, ByteQueue request)
	{
		int req = request.pop<int>();
		if(req == id)
			return ByteQueue::Create<byte>(SUCCESS);
		return ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
	};

	connectee.Accept("client", std::bind(inquiry, -1, std::placeholders::_1));
	connectee.Accept("failure", std::bind(inquiry, -1, std::placeholders::_1));

	for(int i = 0;i<key_num;i++)
	{
		auto connector = std::make_shared<MyConnectorPool>("client", 1, nullptr);
		connector->Connect("localhost", connectee.GetPort(), "server", std::bind(inquiry, i, std::placeholders::_1));
		connectors.push_back(connector);
	}
	for(int i = 0;i<fal_num;i++)
	{
		auto connector = std::make_shared<MyConnectorPool>("failure", 1, nullptr);
		connector->Connect("localhost", connectee.GetPort(), "server", std::bind(inquiry, i, std::placeholders::_1));
		connectors.push_back(connector);
	}

	std::chrono::system_clock::duration timeout_time = std::chrono::milliseconds(5000);
	std::chrono::system_clock::time_point timer_start = std::chrono::system_clock::now();

	while(connectee.GetAuthorized() < key_num + fal_num)	//KeyExchange, Authentication까지 끝날때까지 기다려야한다.
	{
		auto dur = std::chrono::system_clock::now() - timer_start;
		if(dur.count() > timeout_time.count())
		{
			EXPECT_TRUE(false) << "Connection Timeout!";
			return;
		}
	}

	for(int i = 0;i<key_num;i++)
	{
		std::atomic<int> client_count(0);
		std::atomic<int> answer_count(0);
		connectee.Request("client", ByteQueue::Create<int>(i), [&client_count, &answer_count](std::shared_ptr<MyConnector> tor, Expected<ByteQueue, StackErrorCode> result)
		{
			client_count.fetch_add(1);
			if(!result)
				return;
			if(result->pop<byte>() == SUCCESS)
				answer_count.fetch_add(1);
		});
		EXPECT_EQ(client_count.load(), key_num) << client_count.load();
		EXPECT_EQ(answer_count.load(), 1) << answer_count.load();
	}

	for(int i = 0;i<fal_num;i++)
	{
		std::atomic<int> client_count(0);
		std::atomic<int> answer_count(0);
		connectee.Request("failure", ByteQueue::Create<int>(i + fal_num), [&client_count, &answer_count](std::shared_ptr<MyConnector> tor, Expected<ByteQueue, StackErrorCode> result)
		{
			client_count.fetch_add(1);
			if(!result)
				return;
			if(result->pop<byte>() == SUCCESS)
				answer_count.fetch_add(1);
		});
		EXPECT_EQ(client_count.load(), fal_num) << client_count.load();
		EXPECT_EQ(answer_count.load(), 0) << answer_count.load();
	}
}