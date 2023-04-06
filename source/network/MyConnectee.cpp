#include "MyConnectee.hpp"

MyConnectee::MyConnectee(int port, ThreadExceptHandler *parent)
 : t_accept(std::bind(&MyConnectee::AcceptLoop, this), this, false), ClientPool(this), 
	server_socket(port),
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
	ClientPool.safe_loop([&](std::shared_ptr<MyClientSocket> fd){ fd->Close(); });
	ClientPool.Stop();
}

void MyConnectee::Accept(std::string keyword, std::function<ByteQueue(ByteQueue)> process)
{	
	KeywordProcessMap.insert_or_assign(keyword, process);
}

void MyConnectee::AcceptLoop()
{
	Thread::SetThreadName("ConnectAcceptor");

	while(isRunning)
	{
		try
		{
			ClientPool.flush();	//주기적으로(여기선 새 Connector가 들어왔을 때) flush 해줌.
			
			auto client = server_socket.Accept();	
			if (!client)
			{
				if(MyClientSocket::isNormalClose(client.error()))
					break;	//TODO : ServerSocket이 닫혔는지 확인 후, MyConnectee가 가동중이라면 소켓 다시 살리기.
				else
					throw ErrorCodeExcept(client.error(), __STACKINFO__);
			}
			ClientPool.insert(*client, std::bind(&MyConnectee::ClientLoop, this, *client));
		}
		catch(const std::exception &e)
		{
			Logger::log(e.what(), Logger::LogType::error);
			//TODO : socket이 reset이 필요할 경우 reset 해주기. 에러가 발생해서 소켓이 닫혔을 수 있기 때문.
		}
	}
}

void MyConnectee::ClientLoop(std::shared_ptr<MyClientSocket> client)
{
	Thread::SetThreadName("ConnecteeClientLoop_Authenticating");

	//인증용 문자열(즉, 발송지 이름)을 올바르게 받지 못하면 연결을 끊는다.
	auto authenticate = client->Recv();
	if(!authenticate)
	{
		client->Close();
		if(!MyClientSocket::isNormalClose(authenticate.error()))
			throw ErrorCodeExcept(authenticate.error(), __STACKINFO__);
		return;
	}
	std::string keyword = authenticate->popstr();
	if(KeywordProcessMap.find(keyword) == KeywordProcessMap.end())
	{
		Logger::log("Host <- " + keyword + " Failed(Unknown keyword)");
		client->Close();
		return;
	}

	Thread::SetThreadName("ConnecteeClientLoop_" + keyword);

	Logger::log("Host <- " + keyword + " is Connected");	//TODO : Remote Address도 로깅 필요
	StackErrorCode ec = StackErrorCode(client->Send(ByteQueue::Create<byte>(SUCCESS)), __STACKINFO__);
	while(isRunning && ec)
	{
		auto msg = client->Recv();
		if(!msg)
		{
			ec = StackErrorCode(msg.error(), __STACKINFO__);
			break;
		}

		ByteQueue answer;
		try
		{
			ByteQueue data = *msg;
			answer = KeywordProcessMap[keyword](data);
		}
		catch(const std::exception &e)
		{
			answer.Clear();
			answer = ByteQueue::Create<byte>(ERR_PROTOCOL_VIOLATION);
		}
		ec = StackErrorCode(client->Send(answer), __STACKINFO__);
	}

	Logger::log("Host <- " + keyword + " is Disconnected");
	client->Close();
	if(MyClientSocket::isNormalClose(ec))
		throw ErrorCodeExcept(ec, __STACKINFO__);
}