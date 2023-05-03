#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include "Invoker.hpp"

using mylib::utils::Invoker;

TEST(InvokerTest, WaitTest)
{
	Invoker<int> num(10);
	Invoker<bool> sw(false);//For Keeping Process Order
	std::mutex mtx;			//For Keeping Process Order
	Invoker<int> work(0);	//For Keeping Process Order

	std::vector<std::thread> threads;

	threads.push_back(std::thread([&](){
		EXPECT_TRUE(sw.wait(true));
		for(int i = 0;i<5;i++)
		{
			num = num - 1;
			work.wait(num);
		}
		num.release();
	}));

	for(int i = 0;i<10;i++)
	{
		threads.push_back(std::thread([&](int target){
			if(target >= 5)
			{
				std::unique_lock<std::mutex> lk(mtx);
				work = work + 1;
				lk.unlock();
				EXPECT_TRUE(num.wait(target)) << "Wait " << target;
			}
			else
			{
				std::unique_lock<std::mutex> lk(mtx);
				work = work + 1;
				lk.unlock();
				EXPECT_FALSE(num.wait(target)) << "Wait " << target;
			}
			std::unique_lock<std::mutex> lk(mtx);
			work = work - 1;
		}, i));
	}

	work.wait(10);
	sw = true;

	for(auto &t : threads)
	{
		if(t.joinable())
			t.join();
	}
}