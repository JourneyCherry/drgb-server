#include <gtest/gtest.h>
#include <thread>
#include <random>
#include "TimerPool.hpp"

using mylib::threads::TimerPool;

class stopwatch
{
	private:
		std::chrono::system_clock::time_point start;
		std::mutex mtx;
		std::condition_variable cv;
		bool isRunning;
	
	public:
		stopwatch() : isRunning(true)
		{
			start = std::chrono::system_clock::now();
		}

		~stopwatch()
		{
			Close();
		}

		int Lap()
		{
			auto end = std::chrono::system_clock::now();
			auto duration = end - start;
			return duration.count() / 1000000;	//nano -> ms
		}

		void Close()
		{
			isRunning = false;
			cv.notify_all();
		}

		void Wait(std::chrono::system_clock::duration timeout)
		{
			std::unique_lock lk(mtx);
			cv.wait_for(lk, timeout, [&](){return !isRunning; });
		}
};

TEST(TimerPoolTest, BasicTest)
{
	TimerPool pool(1, nullptr);
	auto timer = pool.GetTimer();
	stopwatch watch;
	timer->expires_after(std::chrono::milliseconds(100));
	timer->async_wait([&](boost::system::error_code ec)
	{
		ASSERT_EQ(std::abs(100 - watch.Lap()), 0);
		watch.Close();
	});
	watch.Wait(std::chrono::milliseconds(500));
	pool.ReleaseTimer(timer);
}

void RepeatHandle(std::shared_ptr<boost::asio::steady_timer> timer, stopwatch *watch, int count)
{
	if(timer == nullptr)
		return;
	timer->async_wait([timer, watch, count](boost::system::error_code ec)
	{
		if(ec.failed())
			return;
		ASSERT_EQ(std::abs((10 - count)*100 - watch->Lap()), 0) << count;
		if(count > 0)
		{
			timer->expires_after(std::chrono::milliseconds(100));
			RepeatHandle(timer, watch, count - 1);
		}
		else
			watch->Close();
	});
}

TEST(TimerPoolTest, RepeatTest)
{
	TimerPool pool(1, nullptr);
	auto timer = pool.GetTimer();
	stopwatch watch;
	RepeatHandle(timer, &watch, 10);
	watch.Wait(std::chrono::milliseconds(1000));
	pool.ReleaseTimer(timer);
}

TEST(TimerPoolTest, SizeTest)
{
	TimerPool pool(1, nullptr);
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<> dist(100, 500);

	for(int i = 0;i<100;i++)
	{
		auto timer = pool.GetTimer();

		timer->expires_after(std::chrono::milliseconds(dist(gen)));
		timer->async_wait([&pool, timer](boost::system::error_code error_code)
		{
			pool.ReleaseTimer(timer);
		});
	}
	EXPECT_EQ(pool.Size(), 100);
	while(pool.Size() > 0)
	{}
}

TEST(TimerPoolTest, CorpseTest)
{
	TimerPool pool(1, nullptr);
	auto timer = pool.GetTimer();
	timer->expires_after(std::chrono::milliseconds(100));
	timer->async_wait([&](boost::system::error_code ec){
		ASSERT_TRUE(ec.failed());
	});
	ASSERT_TRUE(pool.is_open());
	ASSERT_EQ(pool.Size(), 1);
	pool.Stop();
	ASSERT_FALSE(pool.is_open());
	ASSERT_EQ(pool.Size(), 0);
	ASSERT_EQ(pool.GetTimer(), nullptr);

	//Released Timer. 해당 타이머는 ioc가 멈췄기 때문에 동작하지 않는다.
	timer->expires_after(std::chrono::milliseconds(100));
	timer->async_wait([&](boost::system::error_code ec){
		ASSERT_FALSE(ec.failed());
	});
}