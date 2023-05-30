#include "MyServerSocket.hpp"

void MyServerSocket::AddClient(std::shared_ptr<MyClientSocket> client)
{
	std::unique_lock<std::mutex> lk(mtx_client);
	client->SetCleanUp(std::bind(&MyServerSocket::RemoveClient, this, std::placeholders::_1));
	clients.push_back(client);
	//ioc.post(std::bind(&MyServerSocket::watcher, this, client));
}

void MyServerSocket::RemoveClient(std::shared_ptr<MyClientSocket> client)
{
	std::unique_lock<std::mutex> lk(mtx_client);
	for(auto iter = clients.begin();iter != clients.end();iter++)
	{
		if(iter->get() == client.get())
		{
			clients.erase(iter);
			return;
		}
	}
}

void MyServerSocket::safe_loop(std::function<void(std::shared_ptr<MyClientSocket>)> process)
{
	std::unique_lock<std::mutex> lk(mtx_client);
	for(auto iter = clients.begin();iter != clients.end(); iter++)
		process(*iter);
}

void MyServerSocket::StartThread()
{
	for(int i = 0;i<thread_count;i++)
	{
		boost::asio::post(threadpool, [&]()
		{
			while(is_open())
			{
				if(ioc.stopped())
					ioc.restart();
				ioc.run();
			}
		});
	}
}

bool MyServerSocket::isRunnable() const
{
	return !ioc.stopped();
}

int MyServerSocket::GetPort() const
{
	return port;
}

size_t MyServerSocket::GetConnected() const
{
	return clients.size();
}

void MyServerSocket::Close()
{
	if(!is_open())
		return;
	CloseSocket();
	ioc.stop();
	threadpool.join();
	//std::unique_lock<std::mutex> lk(mtx_client);
	//clients.clear();
}