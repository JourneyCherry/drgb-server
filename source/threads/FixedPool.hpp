#pragma once
#include <array>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <condition_variable>
#include "ThreadExceptHandler.hpp"
#include "StackTraceExcept.hpp"	//TODO : 상대경로가 아닌 헤더파일만 적으면 vscode에서 인식을 못한다. 단, cmake는 인식함.

using mylib::threads::ThreadExceptHandler;
using mylib::utils::StackTraceExcept;

namespace mylib{
namespace threads{

template <typename T, size_t MAX_POOL>
class FixedPool : public ThreadExceptHandler
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		std::mutex mtx;
		std::condition_variable cv;

		bool isRunning;
		std::atomic<size_t> capacity;

		std::string name;
		std::function<void(T)> process;

		
		std::array<std::thread, MAX_POOL> threadpool;
		std::queue<T> msgqueue;

		void Worker()
		{
			Thread::SetThreadName(name);
			
			while(isRunning)
			{
				ulock lk(mtx);
				cv.wait(lk, [&](){return !isRunning || !msgqueue.empty();});
				if(msgqueue.empty())
					continue;
				T arg = msgqueue.front();
				msgqueue.pop();
				lk.unlock();
				capacity.fetch_sub(1, std::memory_order_acquire);	//이 이후의 명령이 이 앞으로 오는 것 금지.
				try
				{
					process(arg);
				}
				catch(...)
				{
					ThrowThreadException();
				}
				capacity.fetch_add(1, std::memory_order_release);	//이 이전의 명령이 이 뒤로 오는 것 금지.
			}
		}
		
	public:
		FixedPool(std::function<void(T)> _process, ThreadExceptHandler *p = nullptr) : FixedPool("FixedPool", _process, p) {}
		FixedPool(std::string _name, std::function<void(T)> _process, ThreadExceptHandler *p = nullptr) : name(_name), process(_process), isRunning(true), capacity(MAX_POOL)
		{
			SetParent(p);
			for(size_t i = 0;i<MAX_POOL;i++)
				threadpool[i] = std::thread(std::bind(&FixedPool::Worker, this));
		}
		~FixedPool()
		{
			stop();
		}

		bool insert(T arg)
		{
			if(!isRunning)
				return false;

			ulock lk(mtx);
			msgqueue.push(arg);
			lk.unlock();
			cv.notify_one();

			return true;
		}

		void stop()
		{
			isRunning = false;
			cv.notify_all();
			for(size_t i = 0;i<MAX_POOL;i++)
			{
				if(threadpool[i].joinable())
					threadpool[i].join();
			}
		}

		size_t remain()
		{
			return capacity.load(std::memory_order_relaxed);
		}
};

}
}