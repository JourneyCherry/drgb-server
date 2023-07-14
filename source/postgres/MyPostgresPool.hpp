#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>
#include "MyPostgres.hpp"

class MyPostgresPool
{
	private:
		bool isRunning;
		size_t poolSize;
		std::string connection_str;
		int reconnect_delay;	//ms

		//Works like Semaphore
		std::mutex mtx;
		std::condition_variable cv;
		////

		std::vector<std::unique_ptr<pqxx::connection>> connectionPool;
		std::queue<size_t> waitPool;

	public:
		/**
		 * @brief Construct a new MyPostgresPool object
		 * 
		 * @param host Host address of Postgres DB Server.
		 * @param port Port number of Postgres DB Server.
		 * @param db DB Name of Postgres DB Server.
		 * @param user User Name of Postgres DB Server.
		 * @param pwd Password of Postgres DB Server.
		 * @param size Size of Connection Pool.
		 */
		MyPostgresPool(std::string host, int port, std::string db, std::string user, std::string pwd, size_t size);
		~MyPostgresPool();
		/**
		 * @brief Get the Connection object
		 * 
		 * @param timeout Wait for Connection. (ms)
		 * @return std::shared_ptr<MyPostgres> Connection Object. It will be nullptr if failed.
		 */
		std::shared_ptr<MyPostgres> GetConnection(int timeout=-1);

		size_t GetConnectUsage() const;
		size_t GetPoolUsage() const;
};