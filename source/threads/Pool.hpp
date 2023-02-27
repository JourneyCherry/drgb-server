#pragma once
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include "PoolElement.hpp"
#include "MyExpected.hpp"
#include "MyExcepts.hpp"

namespace mylib
{
	namespace threads
	{
		class Pool
		{
			private:
				using ulock = std::unique_lock<std::mutex>;

				using Key = unsigned int;
				using Value = std::shared_ptr<PoolElement>;
				using Thread = std::shared_ptr<std::thread>;

				bool isRunning;

				std::mutex mtx;
				std::condition_variable cv;
				std::queue<std::pair<MyExpected<Value, std::exception_ptr>, Thread>> gc_queue;
				std::map<Key, Thread> pool;

				Key GetKey();

				unsigned int Max_Worker;
				void Worker(Key, Value);

			public:
				Pool(int);
				~Pool();
				void Stop();
				bool insert(Value);
				Value WaitForFinish();
				size_t size() const;
		};
	}
}