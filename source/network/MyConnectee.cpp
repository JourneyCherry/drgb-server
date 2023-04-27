#include "MyConnectee.hpp"

MyConnectee::MyConnectee(std::string keyword, int port, ThreadExceptHandler *parent)
 : t_accept(std::bind(&MyConnectee::AcceptLoop, this), this, false),
	server_socket(port),
	identify_keyword(keyword),
 	ThreadExceptHandler(parent)
{
	isRunning = false;
}

MyConnectee::~MyConnectee()
{
	Close();
}

void MyConnectee::Open()
{
	isRunning = true;
	t_accept.start();
}

void MyConnectee::Close()
{
	isRunning = false;
	server_socket.Close();
	t_accept.join();
	for(auto iter = ConnectorsMap.begin();iter != ConnectorsMap.end();iter++)
	{
		iter->second->safe_loop([&](std::shared_ptr<MyConnector> connector){ connector->Close(); });
		iter->second->Stop();
	}
}

void MyConnectee::Accept(std::string keyword, std::function<ByteQueue(ByteQueue)> process)
{	
	KeywordProcessMap.insert_or_assign(keyword, process);
	ConnectorsMap.insert_or_assign(keyword, std::make_shared<VariadicPool<std::shared_ptr<MyConnector>>>(this));
}

void MyConnectee::AcceptLoop()
{
	Thread::SetThreadName("ConnectAcceptor");

	while(isRunning)
	{
		try
		{
			for(auto iter = ConnectorsMap.begin();iter != ConnectorsMap.end();iter++)
				iter->second->flush();

			auto client = server_socket.Accept();	
			if (!client)
			{
				if(MyClientSocket::isNormalClose(client.error()))
					break;	//TODO : ServerSocket이 닫혔는지 확인 후, MyConnectee가 가동중이라면 소켓 다시 살리기.
				else
					throw ErrorCodeExcept(client.error(), __STACKINFO__);
			}
			auto connector = std::make_shared<MyConnector>(this, *client);
			std::string keyword = connector->keyword;
			if(KeywordProcessMap.find(keyword) == KeywordProcessMap.end())
			{
				connector->Reject(ERR_NO_MATCH_KEYWORD);
				Logger::log("Connectee : Unknwon Keyword(" + keyword + ") " + connector->ToString(), Logger::LogType::info);
				continue;
			}
			connector->SetInquiry(KeywordProcessMap[keyword]);
			ConnectorsMap[keyword]->insert(connector, std::bind(&MyConnector::RecvLoop, connector));
			connector->Accept(identify_keyword);
			Logger::log(identify_keyword + " <- " + keyword + " : " + connector->ToString(), Logger::LogType::info);
		}
		catch(const std::exception &e)
		{
			Logger::log(e.what(), Logger::LogType::error);
			//TODO : socket이 reset이 필요할 경우 reset 해주기. 에러가 발생해서 소켓이 닫혔을 수 있기 때문.
		}
	}
}

void MyConnectee::Request(std::string keyword, ByteQueue request, std::function<void(std::shared_ptr<MyConnector>, ByteQueue)> process)
{
	if(ConnectorsMap.find(keyword) == ConnectorsMap.end())
		return;
	auto connectors = ConnectorsMap[keyword];
	connectors->safe_loop([&](std::shared_ptr<MyConnector> connector)
	{
		auto req_result = connector->Request(request);
		if(req_result)
			process(connector, *req_result);
	});
}