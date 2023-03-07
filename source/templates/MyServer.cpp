#include "MyServer.hpp"

MyServer::MyServer(int port_web, int port_tcp) : isRunning(true), workerpool("ClientProcess", std::bind(&MyServer::ClientProcess, this, std::placeholders::_1), this)
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
	isRunning = true;
	for(Thread &t : acceptors)
		t.start();
	Open();
}

void MyServer::Stop()
{
	isRunning = false;

	for(auto &socket : sockets)
		socket->Close();
	for(auto &acceptor : acceptors)
		acceptor.join();
		
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
		auto client = socket->Accept();
		if(!client)
			break;

		workerpool.insert(*client);
	}
}