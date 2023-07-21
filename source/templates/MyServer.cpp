#include "MyServer.hpp"

MyServer::MyServer(int port)
 : web_server(port, ConfigParser::GetInt("ClientThread", 2)),
	isRunning(false),
	redis(SESSION_TIMEOUT),
	dbpool(
		ConfigParser::GetString("DBAddr", "localhost"),
		ConfigParser::GetInt("DBPort", 5432),
		ConfigParser::GetString("DBName", "drgb_db"),
		ConfigParser::GetString("DBUser", "drgb_user"),
		ConfigParser::GetString("DBPwd", "drgbpassword"),
		ConfigParser::GetInt("DBPoolSize", 10)
	),
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
		ServiceServer->Wait();
	});
	
	auto accept_handle = [this](std::shared_ptr<MyClientSocket> socket, ErrorCode ec) -> void
	{
		if(!ec)
			return;
		
		socket->KeyExchange(std::bind(&MyServer::AcceptProcess, this, std::placeholders::_1, std::placeholders::_2));
	};

	redis.Connect(ConfigParser::GetString("SessionAddr", "localhost"), ConfigParser::GetInt("SessionPort", 6379));
	web_server.StartAccept(accept_handle);
	Open();
}

void MyServer::Stop()
{
	if(!isRunning)
		return;

	isRunning = false;
	web_server.Close();
	ServiceServer->Shutdown();
	if(ServiceThread.joinable())
		ServiceThread.join();
	redis.Close();
		
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

size_t MyServer::GetClientUsage()
{
	return web_server.GetConnected();
}

std::map<std::string, size_t> MyServer::GetConnectUsage()
{
	std::map<std::string, size_t> connected;
	connected.insert({"Redis", redis.is_open()?1:0});
	connected.insert({"Postgres", dbpool.GetConnectUsage()});
	connected.insert({"PostgresPool", dbpool.GetPoolUsage()});

	return connected;
}
