#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include <thread>
#include "MyTCPClient.hpp"
#include "MyTCPServer.hpp"
#include "MyTCPTLSClient.hpp"
#include "MyTCPTLSServer.hpp"
#include "MyWebsocketClient.hpp"
#include "MyWebsocketServer.hpp"
#include "MyWebsocketTLSClient.hpp"
#include "MyWebsocketTLSServer.hpp"

#define CERT_FILE "../../resources/drgb.crt"
#define KEY_FILE "../../resources/drgb.key"
#define TIMEOUT 5
#define PORT 54321
#define CLIENT_NUM 5

class SocketArgument
{
	private:
		std::function<std::shared_ptr<MyServerSocket>()> server_instantiator;
		std::function<std::shared_ptr<MyClientSocket>()> client_instantiator;
		bool encryption;
		std::string argName;

	public:
		SocketArgument(
			std::string n,
			std::function<std::shared_ptr<MyServerSocket>()> server,
			std::function<std::shared_ptr<MyClientSocket>()> client,
			bool require_custom_encryption
		) : argName(n), server_instantiator(server), client_instantiator(client), encryption(require_custom_encryption) {}
		SocketArgument(const SocketArgument& copy){ *this = copy; }

		std::shared_ptr<MyServerSocket> GetServer() const { return server_instantiator(); }
		std::shared_ptr<MyClientSocket> GetClient() const { return client_instantiator(); }
		bool isEncrypt() const { return encryption; }
		std::string GetName() const { return argName; }

		SocketArgument& operator=(const SocketArgument& copy)
		{
			server_instantiator = copy.server_instantiator;
			client_instantiator = copy.client_instantiator;
			encryption = copy.encryption;
			argName = copy.argName;

			return *this;
		}
};

class SocketTestFixture : public ::testing::TestWithParam<SocketArgument>
{
	protected:
		void SetUp() override
		{
			signal(SIGPIPE, SIG_IGN);
			auto arg = GetParam();

			Server = arg.GetServer();
			onEncryption = arg.isEncrypt();

			for(size_t i = 0;i<CLIENT_NUM; i++)
				Clients.push_back(arg.GetClient());
		}


		void Test(std::function<void(std::shared_ptr<MyClientSocket>)> sAlgorithm, std::function<void(std::shared_ptr<MyClientSocket>)> cAlgorithm)
		{
			std::thread server_thread([sAlgorithm, this](){ this->ServerLoop(sAlgorithm); });
			std::vector<std::thread> client_threads;
			for(auto &client : Clients)
				client_threads.push_back(std::thread([cAlgorithm, this](std::shared_ptr<MyClientSocket> pclient){ this->ClientLoop(cAlgorithm, pclient); }, client));

			for(auto &t : client_threads)
				t.join();
			server_thread.join();
		}

		void Test(std::function<void(std::shared_ptr<MyClientSocket>)> algorithm)
		{
			Test(algorithm, algorithm);
		}

		void TearDown() override
		{
			if(Server != nullptr) Server->Close();
			for(auto &client : Clients)
				client->Close();
		}

		void ServerLoop(std::function<void(std::shared_ptr<MyClientSocket>)> algorithm)
		{
			std::vector<std::thread> process;
			size_t count = CLIENT_NUM;
			while(count > 0)
			{
				auto client = Server->Accept();
				EXPECT_TRUE(client.isSuccessed()) << "Server : Accept Failed with " << client.error().message_code();
				count--;
				if(!client)
					continue;
				process.push_back(std::thread([](std::function<void(std::shared_ptr<MyClientSocket>)> func, std::shared_ptr<MyClientSocket> pclient, bool encryption)
				{ 
					auto sec = pclient->KeyExchange(encryption);
					ASSERT_TRUE(sec.isSuccessed()) << "Server : Key Exchange Failed with " << sec.message_code();
					if(!sec)
					{
						pclient->Close();
						return;
					}

					func(pclient);

					pclient->Close();
				}, algorithm, *client, onEncryption));
			}
			for(auto &t : process)
				t.join();
		}

		void ClientLoop(std::function<void(std::shared_ptr<MyClientSocket>)> algorithm, std::shared_ptr<MyClientSocket> socket)
		{
			ASSERT_NE(nullptr, socket);
			if(socket == nullptr)
				throw ErrorCodeExcept(ERR_CONNECTION_CLOSED, __STACKINFO__);
			
			auto sec = socket->Connect("localhost", PORT);
			ASSERT_TRUE(sec.isSuccessed()) << "Client : Connect Failed with " << sec.message_code();
			if(!sec)
			{
				socket->Close();
				return;
			}

			sec = socket->KeyExchange(onEncryption);
			ASSERT_TRUE(sec.isSuccessed()) << "Client : Key Exchange Failed with " << sec.message_code();
			if(!sec)
			{
				socket->Close();
				return;
			}

			algorithm(socket);

			socket->Close();
		}

	public:
		struct PrintToStringParamName
		{
			template <class ParamType>
			std::string operator()(const ::testing::TestParamInfo<ParamType> &info) const
			{
				return static_cast<SocketArgument>(info.param).GetName();
			}
		};

	private:
		std::shared_ptr<MyServerSocket> Server;
		std::vector<std::shared_ptr<MyClientSocket>> Clients;
		bool onEncryption;
};

TEST_P(SocketTestFixture, BasicTest)
{
	std::function<void(std::shared_ptr<MyClientSocket>)> serverProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		auto request = socket->Recv(TIMEOUT);
		ASSERT_TRUE(request.isSuccessed()) << "Server : Recv Failed with " << request.error().message_code();
		if(!request)
			return;
		ByteQueue answer = ByteQueue::Create<byte>((request->pop<byte>() == REQ_STARTMATCH)?ANS_MATCHMADE:ERR_PROTOCOL_VIOLATION);
		auto ec = socket->Send(answer);
		ASSERT_TRUE(ec.isSuccessed()) << "Server : Send Failed with " << ec.message_code();
		if(!ec)
			return;
	};

	std::function<void(std::shared_ptr<MyClientSocket>)> clientProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		ByteQueue request = ByteQueue::Create<byte>(REQ_STARTMATCH);
		auto ec = socket->Send(request);
		ASSERT_TRUE(ec.isSuccessed()) << "Client : Send Failed with " << ec.message_code();
		if(!ec)
			return;

		auto answer = socket->Recv(TIMEOUT);
		ASSERT_TRUE(answer.isSuccessed()) << "Client : Recv Failed with " << answer.error().message_code();
		if(!answer)
			return;
		ASSERT_EQ(answer->pop<byte>(), ANS_MATCHMADE);
	};

	Test(serverProcess, clientProcess);
}

TEST_P(SocketTestFixture, DoubleCloseTest)
{
	std::function<void(std::shared_ptr<MyClientSocket>)> commonProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		const int close_count = 5;
		for(int i = 0;i<close_count;i++)
			socket->Close();
	};

	Test(commonProcess);
}

TEST_P(SocketTestFixture, TimeoutTest)
{
	std::function<void(std::shared_ptr<MyClientSocket>)> serverProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		auto answer = socket->Recv(0.1f);
		ASSERT_FALSE(answer.isSuccessed()) << "Server : Received";
		//EXPECT_TRUE(answer.error().code() == ERR_TIMEOUT || answer.error().code() == ERR_CONNECTION_CLOSED) << "Server : Timeout by " << answer.error().message_code();	//Timeout can return ERR_TIMEOUT or ERR_CONNECTION_CLOSED or any Error.
	};

	std::function<void(std::shared_ptr<MyClientSocket>)> clientProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		auto msg = socket->Recv();
		ASSERT_FALSE(msg.isSuccessed());
		EXPECT_EQ(msg.error().code(), ERR_CONNECTION_CLOSED);
	};

	Test(serverProcess, clientProcess);
}

TEST_P(SocketTestFixture, BigDataTest)
{
	const int data_count = 4096;
	ByteQueue bigdata;
	for(int i = 0;i<data_count;i++)
		bigdata.push<int>(i);
	std::function<void(std::shared_ptr<MyClientSocket>)> serverProcess = [bigdata, data_count](std::shared_ptr<MyClientSocket> socket)
	{
		ByteQueue src = bigdata;
		src.Reset();
		auto data = socket->Recv();
		ASSERT_TRUE(data.isSuccessed()) << "Server : Failed to receive with " << data.error().message_code();
		if(!data)
			return;
		for(int i = 0;i<data_count;i++)
			ASSERT_EQ(data->pop<int>(), src.pop<int>());
	};

	std::function<void(std::shared_ptr<MyClientSocket>)> clientProcess = [bigdata](std::shared_ptr<MyClientSocket> socket)
	{
		auto ec = socket->Send(bigdata);
		ASSERT_TRUE(ec.isSuccessed()) << "Client : Failed to send with " << ec.message_code();
	};

	Test(serverProcess, clientProcess);
}

TEST_P(SocketTestFixture, CongestionTest)
{

	std::function<void(std::shared_ptr<MyClientSocket>)> commonProcess = [](std::shared_ptr<MyClientSocket> socket)
	{
		const int data_count = 2048;
		for(int i = 0;i<data_count;i++)
		{
			auto ec = socket->Send(ByteQueue::Create<int>(i));
			ASSERT_TRUE(ec.isSuccessed()) << "Send Failed with " << ec.message_code();
			if(!ec)
				return;
		}

		for(int i = 0;i<data_count;i++)
		{
			auto msg = socket->Recv(0.1f);
			ASSERT_TRUE(msg.isSuccessed()) << "Recv Failed with " << msg.error().message_code();
			ASSERT_EQ(msg->pop<int>(), i);
		}
	};

	Test(commonProcess);
}

INSTANTIATE_TEST_CASE_P(SocketTest, SocketTestFixture, ::testing::Values(
	SocketArgument("TCP", [](){ return std::make_shared<MyTCPServer>(PORT); }, [](){ return std::make_shared<MyTCPClient>(); }, true),
	SocketArgument("TCPTLS", [](){ return std::make_shared<MyTCPTLSServer>(PORT, CERT_FILE, KEY_FILE); }, [](){ return std::make_shared<MyTCPTLSClient>(); }, false),
	SocketArgument("Websocket", [](){ return std::make_shared<MyWebsocketServer>(PORT); }, [](){ return std::make_shared<MyWebsocketClient>(); }, true),
	SocketArgument("WebsocketTLS", [](){ return std::make_shared<MyWebsocketTLSServer>(PORT, CERT_FILE, KEY_FILE); }, [](){ return std::make_shared<MyWebsocketTLSClient>(); }, false)
), SocketTestFixture::PrintToStringParamName());