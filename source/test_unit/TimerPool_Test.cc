#include <gtest/gtest.h>
#include <thread>
#include "TimerPool.hpp"

using mylib::threads::TimerPool;

class BasicTimerTestFixture : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			for(int i = 0;i<thread_count;i++)
			{
				boost::asio::post(tp, [&]()
				{
					while(isRunning)
					{
						if(ioc.stopped())
							ioc.restart();
						ioc.run();

						if(isRunning)
						{
							std::string result = (ioc.stopped()?"true":"false");
							Show("ioc.run() out. stopped : " + result);
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						}
					}
				});
			}
		}

		void TearDown() override
		{
			isRunning = false;
			work_guard.reset();
			ioc.stop();
			tp.join();
		}

	public:
		BasicTimerTestFixture() : thread_count(10), ioc(thread_count), work_guard(boost::asio::make_work_guard(ioc)), tp(thread_count), isRunning(true) {}
		int thread_count;
		boost::asio::io_context ioc;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
		boost::asio::thread_pool tp;
		bool isRunning;
		std::mutex mtx;

		void Show(std::string str)
		{
			std::unique_lock<std::mutex> lk(mtx);
			std::cout << str << std::endl;
		}
};

TEST_F(BasicTimerTestFixture, IOContextTestWithoutWork)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(BasicTimerTestFixture, IOContextTestWithWork)
{
	boost::asio::steady_timer timer(ioc);
	timer.expires_after(std::chrono::milliseconds(500));
	bool isDone = false;
	timer.async_wait([&isDone](boost::system::error_code ec)
	{
		isDone = true;
	});
	//Show("Work Guard Free");
	//work_guard.reset();	//여기서 work_guard를 해제하면 일을 잡은 thread만 대기하고 나머진 죄다 return한다.
	while(!isDone)
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	Show("Timer Expired");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

TEST_F(BasicTimerTestFixture, InitTimerTest)
{
	boost::asio::steady_timer timer(ioc);
	timer.expires_after(std::chrono::milliseconds(0));	//이부분이 없어도 같은 효과를 낸다.
	timer.async_wait([](boost::system::error_code ec)
	{
		if(ec.failed())
			std::cout << ec.message() << std::endl;
		std::cout << "Timer Finished" << std::endl;
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}