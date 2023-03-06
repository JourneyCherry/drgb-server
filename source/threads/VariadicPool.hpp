#pragma once
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <limits>
#include <condition_variable>
#include "Thread.hpp"
#include "ThreadExceptHandler.hpp"
#include "Expected.hpp"			//TODO : 상대경로가 아닌 헤더파일만 적으면 vscode에서 인식을 못한다. 단, cmake는 인식함.
#include "StackTraceExcept.hpp"	//TODO : 상대경로가 아닌 헤더파일만 적으면 vscode에서 인식을 못한다. 단, cmake는 인식함.

namespace mylib{
namespace threads{

using utils::Expected;
using utils::StackTraceExcept;

template <typename Key>
class VariadicPool : public ThreadExceptHandler
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		using Work = std::function<void()>;

		bool isRunning;

		std::mutex mtx;
		std::condition_variable cv;
		std::queue<std::shared_ptr<std::thread>> gc_queue;
		std::map<Key, std::shared_ptr<std::thread>> pool;

		void Worker(Key key, Work work)
		{
			Thread::SetThreadName("VariadicPool Worker");
			try
			{
				work();
			}
			catch(const std::exception &e)
			{
				ThrowThreadException();
			}

			ulock lk(mtx);
			auto self = pool[key];	//자기 자신의 스레드. shared_ptr이기 때문에 여기 지역변수에 저장되면 살아있게 된다.
			pool.erase(key);
			gc_queue.push(self);
			lk.unlock();

			cv.notify_one();
		}

	public:
		VariadicPool(ThreadExceptHandler *p) : isRunning(true), ThreadExceptHandler(p) {}
		~VariadicPool()
		{
			if(isRunning)
				Stop();
		}

		void Stop()
		{
			isRunning = false;
			cv.notify_all();

			ulock lk(mtx);
			while(pool.size() > 0 || !gc_queue.empty())
			{
				cv.wait(lk, [&](){ return !gc_queue.empty(); });
				auto thrd = gc_queue.front();
				gc_queue.pop();
				lk.unlock();

				if(thrd->joinable())
					thrd->join();

				lk.lock();
			}
		}

		bool insert(Key key, Work work)
		{
			if(!isRunning)
				return false;

			if(pool.find(key) != pool.end())
				return false;
			
			try
			{
				ulock lk(mtx);
				pool.insert({key, std::make_shared<std::thread>(std::bind(&VariadicPool::Worker, this, key, work))});
			}
			catch(StackTraceExcept e)
			{
				e.Propagate(__STACKINFO__);
			}
			catch(const std::exception &e)
			{
				throw StackTraceExcept(e.what(), __STACKINFO__);
			}

			return true;
		}

		void safe_loop(std::function<void(Key)> process)
		{
			ulock lk(mtx);
			for(auto &[key, thrd] : pool)
				process(key);
		}

		void flush()
		{
			ulock lk(mtx);
			while(!gc_queue.empty())
			{
				auto thrd = gc_queue.front();
				gc_queue.pop();
				lk.unlock();

				if(thrd->joinable())
					thrd->join();

				lk.lock();
			}
		}
		
		size_t size() const
		{
			return pool.size();
		}
};

}
}