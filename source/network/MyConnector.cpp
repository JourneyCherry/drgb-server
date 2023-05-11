#include "MyConnector.hpp"

MyConnector::MyConnector(ThreadExceptHandler* parent, std::string addr, int port, std::string key, std::function<ByteQueue(ByteQueue)> inquiry)
	: t_recv(std::bind(&MyConnector::ConnectLoop, this), parent, false), 
	keepTryConnect(true),
	keyword(key),
	target_addr(addr),
	target_port(port),
	inquiry_process(inquiry)
{
	socket = std::make_shared<MyTCPClient>();
}

MyConnector::MyConnector(ThreadExceptHandler* parent, std::shared_ptr<MyClientSocket> sock)
	: keepTryConnect(false), socket(sock)	//MyConnectee에서 Accept한 Connector는 ThreadPool에서 RecvLoop를 돌리므로, 별도의 자체 스레드를 돌리지 않는다.
{
	ErrorCodeExcept::ThrowOnFail(socket->KeyExchange(), __STACKINFO__);
	auto authenticate = socket->Recv(MyClientSocket::TIME_KEYEXCHANGE);
	if(!authenticate)
		throw ErrorCodeExcept(authenticate.error(), __STACKINFO__);
	keyword = authenticate->popstr();
}

MyConnector::~MyConnector()
{
	Close();
}

void MyConnector::ConnectLoop()
{
	while(keepTryConnect)
	{
		isConnected = false;

		try
		{
			ErrorCodeExcept::ThrowOnFail(socket->Connect(target_addr, target_port), __STACKINFO__);
			ErrorCodeExcept::ThrowOnFail(socket->KeyExchange(), __STACKINFO__);

			ByteQueue keywordbyte(keyword.c_str(), keyword.size());
			ErrorCodeExcept::ThrowOnFail(socket->Send(keywordbyte), __STACKINFO__);
			auto authenticate = socket->Recv(MyClientSocket::TIME_KEYEXCHANGE);
			if(!authenticate)
				throw ErrorCodeExcept(authenticate.error(), __STACKINFO__);
			byte authenticate_result = authenticate->pop<byte>();
			if(authenticate_result != SUCCESS)
				throw ErrorCodeExcept(authenticate_result, __STACKINFO__);
			
			target_keyword = authenticate->popstr();

			Logger::log(keyword + " -> " + target_keyword + " : " + socket->ToString(), Logger::LogType::info);
			isConnected = true;

			RecvLoop();
		}
		catch(StackTraceExcept sec)
		{
			isConnected = false;
			Logger::log("Connector(" + keyword + ") Failed : " + sec.what(), Logger::LogType::error);
		}

		std::unique_lock<std::mutex> temp(m_req);
		cv_req.wait_for(temp, std::chrono::seconds(RETRY_WAIT_SEC), [&](){return !keepTryConnect;});
	}
}

void MyConnector::RecvLoop()
{
	StackErrorCode sec;
	while(sec)
	{
		Expected<ByteQueue, ErrorCode> result = socket->Recv();
		if(!result)
		{
			sec = StackErrorCode(result.error(), __STACKINFO__);
			break;
		}

		try
		{
			byte header = result->pop<byte>();
			unsigned int id = result->pop<unsigned int>();
			ByteQueue msg = result->split();
			
			switch(header)
			{
				case INQ_REQUEST:
					{
						ByteQueue answer = inquiry_process(msg);
						answer.push_head<unsigned int>(id);
						answer.push_head<byte>(INQ_ANSWER);
						sec = StackErrorCode(socket->Send(answer), __STACKINFO__);
					}
					break;
				case INQ_ANSWER:
					{
						std::unique_lock<std::mutex> lk(m_req);
						if(answer_map.find(id) != answer_map.end())
							answer_map[id] = msg;
						lk.unlock();

						cv_req.notify_all();
					}
					break;
			}
		}
		catch(const std::exception& e)
		{
			Logger::log(e.what(), Logger::LogType::error);
		}
	}

	if(MyClientSocket::isNormalClose(sec))
		Logger::log("Connector(" + keyword + ") Closed.", Logger::LogType::info);
	else
		Logger::log("Connector(" + keyword + ") Failed : " + sec.message_code(), Logger::LogType::error);
	
	isConnected = false;
}

void MyConnector::Accept(std::string host_keyword)
{
	ByteQueue answer = ByteQueue::Create<byte>(SUCCESS);
	answer += ByteQueue(host_keyword.c_str(), host_keyword.size());
	ErrorCodeExcept::ThrowOnFail(socket->Send(answer), __STACKINFO__);
	isConnected = true;
}

void MyConnector::Reject(errorcode_t ec)
{
	socket->Send(ByteQueue::Create<byte>(ec));
	socket->Close();
	isConnected = false;
}

void MyConnector::Connect()
{
	if(!t_recv.isRunning())
		t_recv.start();
}

void MyConnector::SetInquiry(std::function<ByteQueue(ByteQueue)> inquiry)
{
	inquiry_process = inquiry;
}

void MyConnector::Close()
{
	keepTryConnect = false;
	socket->Close();
	cv_req.notify_all();
	t_recv.join();
}

Expected<ByteQueue, StackErrorCode> MyConnector::Request(ByteQueue data)
{
	if (data.Size() <= 0)
		return StackErrorCode(ERR_OUT_OF_CAPACITY, __STACKINFO__);

	if(!isConnected)
		return StackErrorCode(ERR_CONNECTION_CLOSED, __STACKINFO__);

	ByteQueue answer;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());

	std::unique_lock<std::mutex> lk(m_req);
	unsigned int id = dist(gen);	//Random Value
	while(answer_map.find(id) != answer_map.end())
		id = dist(gen);	//Random Value
	
	answer_map.insert({id, ByteQueue()});
	lk.unlock();

	data.push_head<unsigned int>(id);
	data.push_head<byte>(INQ_REQUEST);
	auto sec = StackErrorCode(socket->Send(data), __STACKINFO__);
	if(!sec)
	{
		lk.lock();
		answer_map.erase(id);
		return sec;
	}

	lk.lock();	//lock이 된 상태에서 cv에서 호출해야함.
	bool isMsgIn = cv_req.wait_for(lk, std::chrono::seconds(TIME_WAIT_ANSWER), [&](){ return answer_map[id].Size() > 0; });
	answer = answer_map[id];
	answer_map.erase(id);
	lk.unlock();

	if(!isMsgIn)
		return StackErrorCode(ERR_TIMEOUT, __STACKINFO__);

	return answer;
}

std::string MyConnector::ToString() const
{
	return socket->ToString();
}