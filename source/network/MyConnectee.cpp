#include "MyConnectee.hpp"

MyConnectee::MyConnectee(std::string keyword, int port, int t, ThreadExceptHandler *parent)
 : except(parent), identify_keyword(keyword), MyTCPServer(port, t)
{
	StartAccept(nullptr);	//enterHandle은 GetClient의 2번째 인수이며, Connectee에선 사용되지 않음. client->Prepare()가 아닌 connector->Connectee()를 사용하기 때문.
}

void MyConnectee::Accept(std::string keyword, std::function<ByteQueue(ByteQueue)> process)
{
	KeywordProcessMap.insert_or_assign(keyword, process);
}

std::shared_ptr<MyClientSocket> MyConnectee::GetClient(boost::asio::ip::tcp::socket& socket, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle)
{
	auto connector = std::make_shared<MyConnector>(std::move(socket));
	connector->Connectee(std::bind(&MyConnectee::ClientAuth, this, std::placeholders::_1, std::placeholders::_2));

	return connector;
}

bool MyConnectee::ClientAuth(std::shared_ptr<MyClientSocket> socket, Expected<std::string, ErrorCode> remote_keyword)
{
	if(!remote_keyword.isSuccessed())
	{
		Logger::log("Connectee : " + socket->ToString() + " Failed with " + remote_keyword.error().message_code(), Logger::LogType::error);
		socket->Close();
		return false;
	}
		
	if(KeywordProcessMap.find(*remote_keyword) == KeywordProcessMap.end())
	{
		Logger::log("Connectee : Unknwon Keyword(" + *remote_keyword + ") " + socket->ToString(), Logger::LogType::info);
		socket->Send(ByteQueue::Create<byte>(ERR_NO_MATCH_KEYWORD));
		socket->Close();
		return false;
	}
	else
	{
		ByteQueue answer = ByteQueue::Create<byte>(SUCCESS);
		answer.push<std::string>(identify_keyword);
		auto ec = socket->Send(answer);
		if(!ec)
		{
			socket->Close();
			return false;
		}

		Logger::log(identify_keyword + " <- " + *remote_keyword + " : " + socket->ToString(), Logger::LogType::info);
		auto connector = std::dynamic_pointer_cast<MyConnector>(socket);
		connector->StartInquiry(KeywordProcessMap[*remote_keyword]);
		
		return true;
	}
}

void MyConnectee::Request(std::string keyword, ByteQueue request, std::function<void(std::shared_ptr<MyConnector>, Expected<ByteQueue, StackErrorCode>)> process)
{
	safe_loop([keyword, request, process](std::shared_ptr<MyClientSocket> client)
	{
		auto connector = std::dynamic_pointer_cast<MyConnector>(client);
		if(connector->keyword != keyword)
			return;

		process(connector, connector->Request(request));
	});
}

size_t MyConnectee::GetAuthorized()
{
	std::atomic<size_t> count = 0;
	safe_loop([&](std::shared_ptr<MyClientSocket> client)
	{
		auto connector = std::dynamic_pointer_cast<MyConnector>(client);
		if(connector->is_open() && connector->isAuthorized())
			count++;
	});
	return count;
}

std::map<std::string, size_t> MyConnectee::GetUsage()
{
	std::map<std::string, size_t> result;
	std::atomic<size_t> disconnected(0);
	std::atomic<size_t> connected(0);
	std::map<std::string, std::atomic<size_t>> authorized;
	for(auto &pair : KeywordProcessMap)
		authorized.insert_or_assign(pair.first, 0);

	safe_loop([&](std::shared_ptr<MyClientSocket> client)
	{
		auto connector = std::dynamic_pointer_cast<MyConnector>(client);
		if(!connector->is_open())
			disconnected.fetch_add(1);
		else if(!connector->isAuthorized())
			connected.fetch_add(1);
		else if(authorized.find(connector->keyword) == authorized.end())
			connected.fetch_add(1);
		else
			authorized[connector->keyword].fetch_add(1);
	});

	for(auto &pair : authorized)
		result.insert({pair.first, pair.second.load()});
	result.insert({"Unauthorized", connected.load()});
	result.insert({"Disconnected", disconnected.load()});

	return result;
}