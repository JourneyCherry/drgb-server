#pragma once
#include <array>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>
#include "PoolElement.hpp"
#include "StackTraceExcept.hpp"

namespace mylib{
namespace threads{

template <class T, size_t MAX_POOL>
class FixedPool
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		std::mutex mtx;
		std::condition_variable cv;
		std::queue<std::pair<size_t, std::exception_ptr>> except_queue;

		bool isRunning;

		std::array<T, MAX_POOL> pool;
		std::array<std::thread, MAX_POOL> tpool;

		void Worker(size_t pos)
		{
			if(pos > MAX_POOL)
				throw StackTraceExcept("Position Exceed Range", __STACKINFO__);

			PoolElement* ppe = (PoolElement*)(&(pool[pos]));

			while(isRunning)
			{
				try
				{
					ppe->Work();
				}
				catch(const std::exception &e)
				{
					ulock lk(mtx);
					except_queue.push(std::current_exception());
					lk.unlock();

					cv.notify_one();
				}
			}
		}
		
	public:
		FixedPool() : isRunning(true)
		{
			static_assert(std::is_base_of_v<PoolElement, T>, "Type must inherit from PoolElement");
			for(size_t i = 0;i<MAX_POOL;i++)
				tpool[i] = std::thread(std::bind(&FixedPool::Worker, this, i));
		}
		~FixedPool()
		{
			stop();
			for(size_t i = 0;i<MAX_POOL;i++)
			{
				if(tpool[i].joinable())
					tpool[i].join();
			}
		}
		T& operator[](size_t pos)
		{
			return std::ref(pool[pos]);
		}
		void stop()
		{
			isRunning = false;
			cv.notify_all();
		}
		void WaitCatch()
		{
			ulock lk(mtx);
			cv.wait(lk, [&](){ return !isRunning || !except_queue.empty();});
			if(!isRunning)
				return;
			std::exception_ptr eptr = except_queue.front();
			except_queue.pop();

			std::rethrow_exception(eptr);
		}
};

}
}