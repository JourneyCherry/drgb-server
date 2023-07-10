#include "MyServer.hpp"

MyServer::MyServer(int web, int tcp)
 : web_server(web, 2), tcp_server(tcp, 2), 	//TODO : thread 수를 core 수에 맞추거나 별도 설정(Configfile)으로 결정 필요.
 isRunning(false),
 MgrService(
	std::bind(&MyServer::GetUsage, this),
	std::bind(&MyServer::CheckAccount, this, std::placeholders::_1),
	std::bind(&MyServer::GetClientUsage, this),
	std::bind(&MyServer::GetConnectUsage, this)
 )
{
	grpc::EnableDefaultHealthCheckService(true);
	ServiceBuilder.AddListeningPort(
		"0.0.0.0:" + std::to_string(ConfigParser::GetInt("Service_Port", 52431)), 
		grpc::InsecureServerCredentials()
	);
	ServiceBuilder.RegisterService(&MgrService);
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
	ServiceServer = ServiceBuilder.BuildAndStart();
	ServiceThread = std::thread([this]()
	{
		this->ServiceServer->Wait();
	});
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
	ServiceServer->Shutdown();
	if(ServiceThread.joinable())
		ServiceThread.join();
		
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

size_t MyServer::GetUsage()
{
	return 0;
}

bool MyServer::CheckAccount(Account_ID_t id)
{
	return false;
}

std::pair<size_t, size_t> MyServer::GetClientUsage()
{
	return {tcp_server.GetConnected(), web_server.GetConnected()};
}

std::map<std::string, size_t> MyServer::GetConnectUsage()
{
	std::map<std::string, size_t> connected;

	return connected;
}
