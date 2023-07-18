#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include <thread>
#include <boost/asio/thread_pool.hpp>
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
#define TIMEOUT 300
#define THREAD_NUM 16
#define CLIENT_NUM 100

class SocketArgument
{
	private:
		std::function<std::shared_ptr<MyServerSocket>()> server_instantiator;
		std::function<std::shared_ptr<MyClientSocket>(boost::asio::io_context&)> client_instantiator;
		bool encryption;
		std::string argName;

	public:
		SocketArgument(
			std::string n,
			std::function<std::shared_ptr<MyServerSocket>()> server,
			std::function<std::shared_ptr<MyClientSocket>(boost::asio::io_context&)> client,
			bool require_custom_encryption
		) : argName(n), server_instantiator(server), client_instantiator(client), encryption(require_custom_encryption) {}
		SocketArgument(const SocketArgument& copy){ *this = copy; }

		std::shared_ptr<MyServerSocket> GetServer() const { return server_instantiator(); }
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::io_context& ioc) const { return client_instantiator(ioc); }
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
			server_count = 0;
			client_count = 0;

			signal(SIGPIPE, SIG_IGN);
			auto arg = GetParam();

			Server = arg.GetServer();
			onEncryption = arg.isEncrypt();
			SetServer(Server);
			server_port = Server->GetPort();

			std::unique_lock<std::mutex> lk(mtx);
			for(size_t i = 0;i<CLIENT_NUM; i++)
			{
				auto client = arg.GetClient(client_ioc);
				std::string thrown = "";
				try
				{
					client->SetCleanUp([&](std::shared_ptr<MyClientSocket> socket)
					{
						client_num.fetch_sub(1);
						client_cv.notify_all();
					});
					SetClient(client);
					client_num.fetch_add(1);
				}
				catch(const std::exception& e)
				{
					thrown = e.what();
				}
				EXPECT_EQ(thrown.length(), 0) << "Client : " << thrown;

				Clients.push_back(client);
			}
		}

		void Test()
		{
			for(int i = 0;i<THREAD_NUM;i++)
				boost::asio::post(client_thread, [this](){ this->ClientLoop(); });

			std::unique_lock lk(mtx);
			client_cv.wait(lk, [this](){return client_num.load() <= 0; });
			Clients.clear();

			for(auto &client : Clients)
				client->Close();
			
			EXPECT_EQ(server_count, CLIENT_NUM);
			EXPECT_EQ(client_count, CLIENT_NUM);
		}

		void TearDown() override
		{
			client_work_guard.reset();
			Server->Close();
			client_thread.join();
		}

		void ClientLoop()
		{
			std::string thrown = "";
			try
			{
				client_ioc.run();
			}
			catch(const std::exception& e)
			{
				thrown = e.what();
			}
			EXPECT_EQ(thrown.length(), 0) << "Client : " << thrown;
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

		void CheckEC(ErrorCode ec)
		{
			if(MyClientSocket::isNormalClose(ec))
				return;
			if(ec.code() == ERR_TIMEOUT)
				return;
			if(ec.code() == boost::asio::error::bad_descriptor)
			{
				//EXPECT_TRUE(ec.isSuccessed()) << ec.message_code();
				return;
			}
			if(ec.code() == boost::asio::error::broken_pipe)
			{
				//EXPECT_TRUE(ec.isSuccessed()) << ec.message_code();
				return;
			}
			if(ec.code() == boost::asio::error::not_connected)
			{
				//EXPECT_TRUE(ec.isSuccessed()) << ec.message_code();
				return;
			}
			if(ec.code() == boost::asio::error::connection_reset)
			{
				//EXPECT_TRUE(ec.isSuccessed()) << ec.message_code();
				return;
			}
			EXPECT_TRUE(ec.isSuccessed()) << ec.message_code();
		}

		SocketTestFixture() : client_thread(THREAD_NUM), client_ioc(THREAD_NUM), client_num(0), client_work_guard(boost::asio::make_work_guard(client_ioc)) {}

	private:
		boost::asio::thread_pool client_thread;
		std::shared_ptr<MyServerSocket> Server;
		std::vector<std::shared_ptr<MyClientSocket>> Clients;
		std::mutex mtx;
		boost::asio::io_context client_ioc;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> client_work_guard;
		std::atomic<size_t> client_num;
		std::condition_variable client_cv;
	
	protected:
		bool onEncryption;
		int server_port;
		std::atomic<int> server_count;
		std::atomic<int> client_count;

		virtual void SetServer(std::shared_ptr<MyServerSocket>) {}
		virtual void SetClient(std::shared_ptr<MyClientSocket>) {}
};

class BasicTestFixture : public SocketTestFixture
{
	protected:
		void SetServer(std::shared_ptr<MyServerSocket> socket) override
		{
			socket->StartAccept([this](std::shared_ptr<MyClientSocket> client, ErrorCode acpt_ec)
			{
				EXPECT_TRUE(acpt_ec.isSuccessed()) << acpt_ec.message_code();
				if(!acpt_ec)
					throw ErrorCodeExcept(acpt_ec, __STACKINFO__);
				client->KeyExchange([this](std::shared_ptr<MyClientSocket> ke_client, ErrorCode ke_ec)
				{
					EXPECT_TRUE(ke_ec.isSuccessed()) << ke_ec.message_code();
					if(!ke_ec)
						throw ErrorCodeExcept(ke_ec, __STACKINFO__);
					ServerAccept(ke_client);
				});
			});
		}

		virtual void ServerAccept(std::shared_ptr<MyClientSocket> socket)
		{
			socket->SetTimeout(TIMEOUT, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
			ServerRecv(socket);
		}

		virtual void ServerRecv(std::shared_ptr<MyClientSocket> socket)
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				client->CancelTimeout();
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				byte req = packet.pop<byte>();
				EXPECT_EQ(req, REQ_STARTMATCH);
				byte ans = (req == REQ_STARTMATCH)?ANS_MATCHMADE:ERR_PROTOCOL_VIOLATION;
				ByteQueue answer = ByteQueue::Create<byte>(ans);
				client->Send(answer);
				server_count.fetch_add(1);
				client->SetTimeout(TIMEOUT, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
				ServerRecv(client);
			});
		}

		void SetClient(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->Connect("localhost", server_port, [this](std::shared_ptr<MyClientSocket> client, ErrorCode acpt_ec)
			{
				EXPECT_TRUE(acpt_ec.isSuccessed()) << acpt_ec.message_code();
				if(!acpt_ec)
					throw ErrorCodeExcept(acpt_ec, __STACKINFO__);
				client->KeyExchange([this](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
				{
					EXPECT_TRUE(ke_ec.isSuccessed()) << ke_ec.message_code();
					if(!ke_ec)
						throw ErrorCodeExcept(ke_ec, __STACKINFO__);
					ClientAccept(ke_socket);
				});
			});
		}

		virtual void ClientAccept(std::shared_ptr<MyClientSocket> socket)
		{
			socket->Send(ByteQueue::Create<byte>(REQ_STARTMATCH));
			socket->SetTimeout(TIMEOUT, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
			ClientRecv(socket);
		}

		virtual void ClientRecv(std::shared_ptr<MyClientSocket> socket)
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				client->CancelTimeout();
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				byte answer = packet.pop<byte>();
				EXPECT_EQ(answer, ANS_MATCHMADE);
				client_count.fetch_add(1);
				client->Close();
			});
		}
};

TEST_P(BasicTestFixture, BasicTest)
{
	Test();
}


class ServerCloseTestFixture : public BasicTestFixture
{
	protected:
		void ServerAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->SetTimeout(TIMEOUT / 2, [](std::shared_ptr<MyClientSocket> socket){socket->Close();});
			ServerRecv(socket);
			server_count.fetch_add(1);
		}

		void ServerRecv(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				client->Close();	//여기서 server_count를 올리면 timeout때 1번, Closed때 1번 총 2번 올라가게 되므로, 여기서 올리면 안된다.
			});
		}

		void ClientAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			ClientRecv(socket);
		}

		void ClientRecv(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				if(!ec)
				{
					CheckEC(ec);
					client_count.fetch_add(1);
					client->Close();
					return;
				}
				ClientRecv(client);
			});
		}
};

TEST_P(ServerCloseTestFixture, ServerCloseTest)
{
	Test();
}

class TimeoutTestFixture : public BasicTestFixture
{
	protected:
		void ServerAccept(std::shared_ptr<MyClientSocket> socket) override { CommonAccept(socket, true); }
		void ClientAccept(std::shared_ptr<MyClientSocket> socket) override { CommonAccept(socket, false); }

		void CommonAccept(std::shared_ptr<MyClientSocket> socket, bool isServer)
		{
			if(isServer)
				socket->SetTimeout(TIMEOUT, [this](std::shared_ptr<MyClientSocket> socket)
				{
					server_count.fetch_add(1);
					socket->Send(ByteQueue::Create<byte>(ERR_TIMEOUT));
					socket->Close();
				});
			CommonRecv(socket);
		}

		void CommonRecv(std::shared_ptr<MyClientSocket> socket)
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				client->CancelTimeout();
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				byte receive_byte = packet.pop<byte>();
				EXPECT_EQ(receive_byte, ERR_TIMEOUT);
				client_count.fetch_add(1);
				client->Close();
			});
		}
};

TEST_P(TimeoutTestFixture, TimeoutTest)
{
	Test();
}

class BigDataTestFixture : public BasicTestFixture
{
	private:
		ByteQueue src_bigdata;
	protected:
		void ServerAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			ServerRecv(socket);
		}

		void ServerRecv(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				ByteQueue bigdata = src_bigdata;
				EXPECT_EQ(packet.Size(), bigdata.Size());
				int data_count = bigdata.Size() / sizeof(int);
				for(int i = 0;i<data_count;i++)
				{
					int lhs = packet.pop<int>();
					int rhs = bigdata.pop<int>();
					EXPECT_EQ(lhs, rhs) << i << "th data";
					if(lhs != rhs)
					{
						client->Send(ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION));
						client->Close();
						return;
					}
				}
				server_count.fetch_add(1);
				client->Send(ByteQueue::Create<byte>(SUCCESS));
				client->Close();
			});
		}

		void ClientAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			ByteQueue bigdata = src_bigdata;
			socket->Send(bigdata);
			ClientRecv(socket);
		}

		void ClientRecv(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				byte answer = packet.pop<byte>();
				if(answer == SUCCESS)
					client_count.fetch_add(1);
				client->Close();
			});
		}
	public:
		void AddData(ByteQueue data)
		{
			src_bigdata = data;
		}
};

TEST_P(BigDataTestFixture, BigDataTest)
{
	constexpr int block_size = 1024;
	constexpr int data_count = (block_size / sizeof(int)) * 10;	//TODO : 이게 커지면 마지막 fragment가 버려짐.
	ByteQueue bigdata;
	for(int i = 0;i<data_count;i++)
		bigdata.push<int>(i);
	
	AddData(bigdata);

	Test();
}

class CongestionTestFixture : public BasicTestFixture
{
	private:
		static constexpr int data_count = 100;	//TODO : 이게 2이상 커지면 실패한다.
	protected:
		void ServerAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			ServerRecv(socket, std::make_shared<int>(0));
		}

		void ServerRecv(std::shared_ptr<MyClientSocket> socket, std::shared_ptr<int> progress)
		{
			socket->StartRecv([this, progress](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				if(!ec)
				{
					CheckEC(ec);
					EXPECT_EQ(*progress, data_count) << "Server : " << ec.message_code();
					client->Close();
					return;
				}
				int data = packet.pop<int>();
				EXPECT_EQ(data, *progress + 1) << "Server : " << data;
				if(data != *progress + 1)
				{
					client->Close();
					return;
				}
				else
				{
					*progress = data;
				}
				if(*progress == data_count)
				{
					server_count.fetch_add(1);
					client->Send(ByteQueue::Create<byte>(SUCCESS));
				}
				ServerRecv(client, progress);
			});
		}

		void ClientAccept(std::shared_ptr<MyClientSocket> socket) override
		{
			for(int i = 1;i<=data_count;i++)
			{
				ErrorCode ec = socket->Send(ByteQueue::Create<int>(i));
				if(!ec)
				{
					EXPECT_TRUE(ec.isSuccessed()) << "Client : " << ec.message_code();
					break;
				}
			}
			ClientRecv(socket);
		}

		void ClientRecv(std::shared_ptr<MyClientSocket> socket) override
		{
			socket->StartRecv([this](std::shared_ptr<MyClientSocket> client, ByteQueue packet, ErrorCode ec)
			{
				if(!ec)
				{
					CheckEC(ec);
					client->Close();
					return;
				}
				byte answer = packet.pop<byte>();
				EXPECT_EQ(answer, SUCCESS) << "Client : " << (int)answer;
				if(answer == SUCCESS)
					client_count.fetch_add(1);
				client->Close();
			});
		}
};

TEST_P(CongestionTestFixture, CongestionTest)
{
	Test();
}

boost::asio::ssl::context sslctx{boost::asio::ssl::context::tlsv12};
auto parameters = ::testing::Values(
	SocketArgument("TCP", [](){ return std::make_shared<MyTCPServer>(0, THREAD_NUM); }, [](boost::asio::io_context& cioc){ return std::make_shared<MyTCPClient>(boost::asio::ip::tcp::socket(cioc)); }, true),
	SocketArgument("TCPTLS", [](){ return std::make_shared<MyTCPTLSServer>(0, THREAD_NUM, CERT_FILE, KEY_FILE); }, [](boost::asio::io_context& cioc){ return std::make_shared<MyTCPTLSClient>(sslctx, boost::asio::ip::tcp::socket(cioc)); }, false),
	SocketArgument("Websocket", [](){ return std::make_shared<MyWebsocketServer>(0, THREAD_NUM); }, [](boost::asio::io_context& cioc){ return std::make_shared<MyWebsocketClient>(boost::asio::ip::tcp::socket(cioc)); }, true),
	SocketArgument("WebsocketTLS", [](){ return std::make_shared<MyWebsocketTLSServer>(0, THREAD_NUM, CERT_FILE, KEY_FILE); }, [](boost::asio::io_context& cioc){ return std::make_shared<MyWebsocketTLSClient>(sslctx, boost::asio::ip::tcp::socket(cioc)); }, false)
);

INSTANTIATE_TEST_CASE_P(SocketTest, BasicTestFixture, parameters, SocketTestFixture::PrintToStringParamName());
INSTANTIATE_TEST_CASE_P(SocketTest, ServerCloseTestFixture, parameters, SocketTestFixture::PrintToStringParamName());
INSTANTIATE_TEST_CASE_P(SocketTest, TimeoutTestFixture, parameters, SocketTestFixture::PrintToStringParamName());
INSTANTIATE_TEST_CASE_P(SocketTest, BigDataTestFixture, parameters, SocketTestFixture::PrintToStringParamName());
INSTANTIATE_TEST_CASE_P(SocketTest, CongestionTestFixture, parameters, SocketTestFixture::PrintToStringParamName());