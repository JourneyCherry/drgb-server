#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

bool isRunning;
std::chrono::system_clock::time_point start_time;
std::chrono::system_clock::time_point middle_time;
std::chrono::system_clock::time_point end_time;
std::chrono::system_clock::time_point overrun_time;

void callback(boost::asio::steady_timer &timer, const boost::system::error_code &ec)
{
	if(ec == boost::asio::error::operation_aborted)
	{
		middle_time = std::chrono::system_clock::now();
		timer.async_wait(std::bind(callback, std::ref(timer), std::placeholders::_1));
		//expires_after()를 호출하지 않고 async_wait()을 호출하면 남은 시간 후 호출 된다.
		return;
	}
	end_time = std::chrono::system_clock::now();
}

TEST(BoostTimerTest, DoubleClickTest)
{
	boost::asio::io_context ioc;
	boost::asio::steady_timer timer(ioc);
	std::thread t([&]()
	{
		while(isRunning)
		{
			if(ioc.stopped())
				ioc.restart();
			ioc.run();
		}
	});

	isRunning = true;
	start_time = std::chrono::system_clock::now();

	timer.expires_after(std::chrono::milliseconds(1000));
	timer.async_wait(std::bind(callback, std::ref(timer), std::placeholders::_1));
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	timer.cancel();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	timer.async_wait([&](const boost::system::error_code& ec)
	{
		EXPECT_FALSE(ec.failed()) << ec.message();
		overrun_time = std::chrono::system_clock::now();
		//expire 된 후, expires_after()를 호출하지 않고 async_wait()을 호출하면 즉시 호출된다.
	});
	isRunning = false;
	if(t.joinable())
		t.join();
	
	auto dur_mid = std::chrono::round<std::chrono::milliseconds>(middle_time - start_time);
	auto dur_total = std::chrono::round<std::chrono::milliseconds>(end_time - start_time);
	auto dur_over = std::chrono::round<std::chrono::milliseconds>(overrun_time - start_time);

	EXPECT_EQ(dur_mid.count(), 500) << "Mid Duration : " << dur_mid.count() << " ms";
	EXPECT_EQ(dur_total.count(), 1000) << "Total Duration : " << dur_total.count() << " ms";
	EXPECT_EQ(dur_over.count(), 1500) << "Overrun Duration : " << dur_over.count() << " ms";
}