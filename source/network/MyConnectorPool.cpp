#include "MyConnectorPool.hpp"

MyConnectorPool::MyConnectorPool(std::string keyword_, const int &t, ThreadExceptHandler* p) : isRunning(true), keyword(keyword_), ioc(t), threadloop(t), ThreadExceptHandler(p)
{
	int thread_num = (t<=0)?1:t;
	for(int i = 0;i<thread_num;i++)
	{
		boost::asio::post(threadloop, std::bind(&MyConnectorPool::Run, this));
	}
}

MyConnectorPool::~MyConnectorPool()
{
	Close();
}

void MyConnectorPool::Run()
{
	while(isRunning)
	{
		try
		{
			if(ioc.stopped())
				ioc.restart();
			ioc.run();
		}
		catch(...)
		{
			ThrowThreadException(std::current_exception());
		}
	}
}

void MyConnectorPool::Reconnect(std::shared_ptr<MyClientSocket> socket, std::string remote_keyword, std::function<ByteQueue(ByteQueue)> process)
{
	auto result = pool.EraseLKey(remote_keyword);

	if(!isRunning || !result.isSuccessed())
		return;
	
	//TODO : Do after RETRY_WAIT_SEC time.
	auto info = std::get<1>(*result);
	std::string addr = std::get<0>(info);
	int port = std::get<1>(info);

	Logger::log("Connector(" + remote_keyword + ") is Closed.", Logger::LogType::error);
	auto timer = std::make_shared<boost::asio::steady_timer>(ioc, std::chrono::milliseconds(RETRY_WAIT_SEC));
	timer->async_wait([this, addr, port, remote_keyword, process, timer](boost::system::error_code error_code)
	{
		if(error_code.failed())
			return;
		this->Connect(addr ,port, remote_keyword, process);
	});
}

void MyConnectorPool::Connect(std::string addr, int port, std::string remote_keyword, std::function<ByteQueue(ByteQueue)> process)
{
	if(!isRunning)
		return;

	auto connector = std::make_shared<MyConnector>(boost::asio::ip::tcp::socket(ioc));
	connector->SetCleanUp(std::bind(&MyConnectorPool::Reconnect, this, std::placeholders::_1, remote_keyword, process));
	connector->Connect(addr, port, [this, remote_keyword, connector, process](std::shared_ptr<MyClientSocket> con_socket, ErrorCode con_ec)
	{
		if(!con_ec)
		{
			con_socket->Close();
			return;
		}
		connector->Connector(keyword, [this, remote_keyword, connector, process](std::shared_ptr<MyClientSocket> tor_socket, Expected<std::string, ErrorCode> receive_keyword)
		{
			if(!receive_keyword.isSuccessed())
			{
				Logger::log("Connector(" + tor_socket->ToString() + ") Failed : " + receive_keyword.error().message_code(), Logger::LogType::error);
				tor_socket->Close();
				return;
			}
			if(remote_keyword != *receive_keyword)
			{
				Logger::log("Not Expected Keyword : " + *receive_keyword, Logger::LogType::error);
				tor_socket->Close();
			}
			else
			{
				Logger::log(keyword + " -> " + *receive_keyword + " : " + tor_socket->ToString(), Logger::LogType::info);
				connector->StartInquiry(process);
			}
		});
	});
	keywords.push_back(remote_keyword);
	pool.Insert(remote_keyword, std::make_tuple(addr, port), connector);
}

Expected<ByteQueue, StackErrorCode> MyConnectorPool::Request(std::string remote_keyword, ByteQueue request)
{
	auto conn = pool.FindLKey(remote_keyword);
	if(!conn.isSuccessed())
		return StackErrorCode(ERR_CONNECTION_CLOSED, __STACKINFO__);

	return conn->second->Request(request);
}

void MyConnectorPool::Close()
{
	isRunning = false;
	for(auto &key : keywords)
	{
		auto result = pool.FindLKey(key);
		if(!result)
			continue;
		result->second->Close();
	}
	threadloop.join();
}

size_t MyConnectorPool::GetConnected() const
{
	return pool.Size();
}

size_t MyConnectorPool::GetAuthorized()
{
	size_t count = 0;
	for(auto &key : keywords)
	{
		auto info = pool.FindLKey(key);
		if(!info)
			continue;
		auto connector = std::get<1>(*info);
		if(connector->is_open() && connector->isAuthorized())
			count++;
	}
	return count;
}