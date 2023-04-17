#include "MyServer.hpp"

MyServer::MyServer(int port_web, int port_tcp) : isRunning(false), workerpool("ClientProcess", std::bind(&MyServer::ClientProcess, this, std::placeholders::_1), this)
{
	sockets[0] = std::make_shared<MyTCPServer>(port_tcp);
	sockets[1] = std::make_shared<MyWebsocketServer>(port_web);

	for(int i = 0;i<MAX_SOCKET;i++)
		acceptors[i] = Thread(std::bind(&MyServer::Accept, this, sockets[i]), this, false);
}

MyServer::~MyServer()
{
	if(isRunning)
		Stop();
}

void MyServer::Start()
{
	if(isRunning)
		return;

	isRunning = true;

	for(Thread &t : acceptors)
		t.start();
	Open();
}

void MyServer::Stop()
{
	if(!isRunning)
		return;

	isRunning = false;

	for(auto &socket : sockets)
		socket->Close();
	for(auto &acceptor : acceptors)
		acceptor.join();
	workerpool.safe_loop([&](std::shared_ptr<MyClientSocket> client)
	{
		client->Close();
	});
	workerpool.stop();
		
	Close();
	GracefulWakeUp();
}

void MyServer::Join()
{
	while(isRunning)
	{
		try
		{
			WaitForThreadException();
		}
		catch(const std::exception &e)
		{
			Logger::log(e.what(), Logger::LogType::error);
		}
	}
}

void MyServer::Accept(std::shared_ptr<MyServerSocket> socket)
{
	Thread::SetThreadName("ServerAcceptor");

	while(isRunning)
	{
		try
		{
			auto client = socket->Accept();
			if(!client)
			{
				if(MyClientSocket::isNormalClose(client.error()))
					break;
				else
					throw ErrorCodeExcept(client.error(), __STACKINFO__);
			}

			workerpool.insert(*client);
		}
		catch(const std::exception& e)
		{
			Logger::log(e.what(), Logger::LogType::error);	//에러 발생해도 스레드를 죽이지 않고 그대로 진행.
			//TODO : socket이 reset이 필요할 경우 reset 해주기. 에러가 발생해서 소켓이 닫혔을 수 있기 때문.
		}
	}
}