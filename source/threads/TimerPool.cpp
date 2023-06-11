#include "TimerPool.hpp"

namespace mylib{
namespace threads{

TimerPool::TimerPool(const size_t &thread_num, ThreadExceptHandler *p) : isRunning(true), ioc(thread_num), ThreadPool(thread_num), ThreadExceptHandler(p)
{
	for(int i = 0;i<thread_num;i++)
	{
		boost::asio::post(ThreadPool, [this]()
		{
			while(this->isRunning)
			{
				try
				{
					if(this->ioc.stopped())
						this->ioc.restart();
					this->ioc.run();
				}
				catch(...)
				{
					this->ThrowThreadException(std::current_exception());
				}
			}
		});
	}
}

TimerPool::~TimerPool()
{
	if(isRunning)
		Stop();
}

void TimerPool::Stop()
{
	isRunning = false;
	boost::system::error_code error_code;
	for(auto &timer : TimerSet)
		timer->cancel(error_code);	//에러 무시
	TimerSet.clear();
	ioc.stop();
	ThreadPool.join();
}

std::shared_ptr<boost::asio::steady_timer> TimerPool::GetTimer()
{
	if(!isRunning)
		return nullptr;

	auto timer = std::make_shared<boost::asio::steady_timer>(ioc);

	std::unique_lock<std::mutex> lk(timer_mtx);
	auto result = TimerSet.insert(timer);
	lk.unlock();
	if(result.second == false || result.first == TimerSet.end())
		return nullptr;

	return timer;
}

void TimerPool::ReleaseTimer(const std::shared_ptr<boost::asio::steady_timer>& timer)
{
	if(!isRunning)
		return;

	std::unique_lock<std::mutex> lk(timer_mtx);
	TimerSet.erase(timer);
}

size_t TimerPool::Size() const
{
	return TimerSet.size();
}

bool TimerPool::is_open() const
{
	return isRunning;
}

}
}