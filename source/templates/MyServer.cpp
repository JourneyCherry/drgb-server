#include "MyServer.hpp"

MyServer::MyServer(int port_web, int port_tcp) : isRunning(true)
{
	sockets[0] = std::make_shared<MyTCPServer>(port_tcp);
	sockets[1] = std::make_shared<MyWebsocketServer>(port_web);

	for(int i = 0;i<MAX_SOCKET;i++)
		acceptors[i] = Thread(std::bind(&MyServer::Accept, this, std::placeholders::_1, sockets[i]), this, false);
	
	v_workers.reserve(MAX_CLIENTS);
	for(int i = 0;i<MAX_CLIENTS;i++)
		v_workers.push_back(std::make_shared<Thread>(std::bind(&MyServer::Work, this, std::placeholders::_1), this, false));
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
	for(std::shared_ptr<Thread> &t : v_workers)
		t->start();
	Open();
}

void MyServer::Stop()
{
	isRunning = false;

	for(auto &socket : sockets)
		socket->Close();
	cv_workers.notify_all();
	for(auto &acceptor : acceptors)
		acceptor.stop();
		
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
		catch(...)
		{
			Logger::raise();
			//TODO : exception 발생시킨 곳 확인하고 다시 서비스 올리기.
		}
	}
}

void MyServer::Accept(std::shared_ptr<bool> killswitch, std::shared_ptr<MyServerSocket> socket)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "ServerAcceptor");
#endif
	while(isRunning && !(*killswitch))
	{
		auto client = socket->Accept();
		if(!client)
		{
			if(client.error() < 0)	//-1은 shutdown에 의해서 종료될 경우이다. 0이상은 errno 또는 에러코드 값.
				break;
			else
				throw StackTraceExcept("Socket Accept Failed (" + std::to_string(client.error()) + ")", __STACKINFO__);
		}

		ulock lk(m_accepts);
		q_accepts.push(*client);
		lk.unlock();
		cv_workers.notify_one();
	}
}

void MyServer::Work(std::shared_ptr<bool> killswitch)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), "ServerWorker");
#endif
	while(isRunning && !(*killswitch))
	{
		ulock lk(m_accepts);
		cv_workers.wait(lk, [&](){return !q_accepts.empty() || !isRunning || *killswitch;});
		if(!isRunning || *killswitch)
			break;
		auto client = q_accepts.front();
		q_accepts.pop();
		lk.unlock();

		try
		{
			ClientProcess(client);
		}
		catch(StackTraceExcept e)
		{
			e.stack(__STACKINFO__);
			Logger::raise(std::current_exception());	//Release면 Log 남기기, Debug면 Throw
		}
	}
}