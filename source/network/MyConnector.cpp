#include "MyConnector.hpp"

MyConnector::MyConnector(ThreadExceptHandler* parent, std::string addr, int port, std::string key)
	: t_conn(std::bind(&MyConnector::ConnectLoop, this), parent, false)
{
	isRunning = false;
	isConnecting = false;
	target_addr = addr;
	target_port = port;
	keyword = key;
}

MyConnector::~MyConnector()
{
	Disconnect();
}

void MyConnector::Connect()
{
	if(!isRunning)
	{
		isRunning = true;
		if(keyword.length() <= 0)
			throw StackTraceExcept("MyConnector::Create_Socket() : keyword is null", __STACKINFO__);
		if(!isConnecting)
			t_conn.start();
	}
}

void MyConnector::ConnectLoop()
{
	isConnecting = true;
	Thread::SetThreadName("Connector");

	auto waitforsec = [&](){

	};
	
	while(isRunning)
	{
		try
		{
			std::unique_lock<std::mutex>(m_req);
			StackErrorCode sec = client_socket.Connect(target_addr, target_port);
			if(!sec)
				throw ErrorCodeExcept(sec);

			ByteQueue keyword_bytes(keyword.c_str(), keyword.size());
			ErrorCode ec = client_socket.Send(keyword_bytes);
			if(!ec)
				throw ErrorCodeExcept(ec, __STACKINFO__);
			auto result = client_socket.Recv();
			if(!result)
				throw ErrorCodeExcept(result.error(), __STACKINFO__);

			byte header = result->pop<byte>();
			if(header != SUCCESS)
				throw StackTraceExcept("Failed to Authenticate", __STACKINFO__);

			//Logger::log("MyConnector::Connect(" + target_addr + ") : success as " + keyword, Logger::LogType::debug);
			Logger::log("Host -> " + target_addr + " is Connected");
			break;
		}
		catch(const std::exception &e)
		{
			Logger::log(e.what(), Logger::LogType::error);
		}

		client_socket.Close();
		
		Logger::log("MyConnector::Connect() : failed. retry after " + std::to_string(RETRY_WAIT_SEC) + " sec", Logger::LogType::debug);
		std::this_thread::sleep_for(std::chrono::seconds(RETRY_WAIT_SEC));
	}
	isConnecting = false;
	cv.notify_all();
}

void MyConnector::Disconnect()
{
	isRunning = false;
	t_conn.join();
	cv.notify_all();
	//TODO : t_conn의 exception 처리하기. 단, rethrow를 하면 소멸자에서 예외를 던질 수 있으므로, Logging을 통해서 처리하자.
}

ByteQueue MyConnector::Request(ByteQueue data)
{
	if (data.Size() <= 0)
		throw StackTraceExcept("Request data is empty", __STACKINFO__);

	if(!isRunning)
		throw StackTraceExcept("Connector is Closed", __STACKINFO__);
	
	StackErrorCode ec(ERR_CONNECTION_CLOSED, __STACKINFO__);
	while(isRunning)
	{
		std::unique_lock<std::mutex> lk(m_req);
		cv.wait(lk, [&](){return !isConnecting || !isRunning;});	//TODO : Timeout 넣기.
		ec = StackErrorCode(client_socket.Send(data), __STACKINFO__);
		if(!ec)
		{
			Connect();
			continue;
		}

		auto msg = client_socket.Recv();
		if(!msg)
		{
			ec = StackErrorCode(msg.error(), __STACKINFO__);
			Connect();
			continue;
		}

		return ByteQueue(msg.value());
	}
	throw ErrorCodeExcept(ec, __STACKINFO__);
}