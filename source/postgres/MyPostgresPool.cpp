#include "MyPostgresPool.hpp"

MyPostgresPool::MyPostgresPool(
	std::string host, 
	int port, 
	std::string db, 
	std::string user, 
	std::string pwd, 
	size_t size
)
 : isRunning(true), poolSize(size),
 connection_str("host=" + host + " dbname=" + db + " user=" + user + " password=" + pwd + " port=" + std::to_string(port))
{
	connectionPool.reserve(poolSize);
	for(size_t i = 0;i<poolSize;i++)
	{
		try
		{
			auto conn = std::make_unique<pqxx::connection>(connection_str);
			connectionPool.push_back(std::move(conn));
		}
		catch(...)
		{
			if(connectionPool.size() <= i)
				connectionPool.push_back(nullptr);
		}
		waitPool.push(i);
	}
}

MyPostgresPool::~MyPostgresPool()
{
	isRunning = false;
	cv.notify_all();
}

std::shared_ptr<MyPostgres> MyPostgresPool::GetConnection(int timeout)	//ms
{
	static const auto predicate = [this](){ return !waitPool.empty() || !isRunning; };
	std::unique_lock lk(mtx);
	if(timeout >= 0)
	{
		bool available = cv.wait_for(lk, std::chrono::milliseconds(timeout), predicate);
		if(!available)
			return nullptr;
	}
	else
		cv.wait(lk, predicate);
	
	if(!isRunning)
		return nullptr;
	
	size_t ptr = waitPool.front();
	if(connectionPool[ptr] == nullptr || !connectionPool[ptr]->is_open())
	{
		try
		{
			auto conn = std::make_unique<pqxx::connection>(connection_str);
			connectionPool[ptr] = std::move(conn);
		}
		catch(...)
		{
			connectionPool[ptr] = nullptr;
			return nullptr;
		}
		if(!connectionPool[ptr]->is_open())
			return nullptr;
	}
	waitPool.pop();
	lk.unlock();
	

	return std::make_shared<MyPostgres>(connectionPool[ptr].get(), [this, ptr]() -> void
	{
		std::unique_lock lk(mtx);
		waitPool.push(ptr);
		lk.unlock();
		cv.notify_one();
	});
}

size_t MyPostgresPool::GetConnectUsage() const
{
	size_t count = 0;
	for(auto &conn : connectionPool)
	{
		if(conn != nullptr && conn->is_open())
			count++;
	}

	return count;
}

size_t MyPostgresPool::GetPoolUsage() const
{
	return poolSize - waitPool.size();
}