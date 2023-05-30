#include "MyServer.hpp"

MyServer::MyServer(int web, int tcp) : web_server(web, 2), tcp_server(tcp, 2), isRunning(false)	//TODO : thread 수를 core 수에 맞추거나 별도 설정(Configfile)으로 결정 필요.
{
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
	web_server.StartAccept(std::bind(&MyServer::AcceptProcess, this, std::placeholders::_1, std::placeholders::_2));
	tcp_server.StartAccept(std::bind(&MyServer::AcceptProcess, this, std::placeholders::_1, std::placeholders::_2));
	Open();
}

void MyServer::Stop()
{
	if(!isRunning)
		return;

	isRunning = false;
	web_server.Close();
	tcp_server.Close();
		
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