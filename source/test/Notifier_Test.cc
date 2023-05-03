#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "Notifier.hpp"

using mylib::utils::Notifier;
using mylib::utils::NotifyTarget;

class TestTarget_1 : public NotifyTarget
{
	public:
		int data;
	public:
		TestTarget_1(int d) : data(d){}
		int Type() override
		{
			return 1;
		}
};

class TestTarget_2 : public NotifyTarget
{
	public:
		int data;
	public:
		TestTarget_2(int d) : data(d){}
		int Type() override
		{
			return 2;
		}
};

TEST(NotifierTest, NotifyTest)
{
	std::thread t;

	{
		std::mutex mtx;				//For Keeping Process Order
		std::condition_variable cv;	//For Keeping Process Order
		int count = 0;				//For Keeping Process Order
		
		Notifier noti;
		t = std::thread([&](){
			while(auto notification = noti.wait())
			{
				count++;
				cv.notify_all();
				switch((*notification)->Type())
				{
					case 1:
						EXPECT_NEAR(1, ((TestTarget_1*)(notification->get()))->data, 1);
						break;
					case 2:
						EXPECT_NEAR(1, ((TestTarget_2*)(notification->get()))->data, 1);
						break;
				}
			}
			EXPECT_EQ(6, count);
			cv.notify_all();
		});
		for(int i = 0;i<3;i++)
			noti.push(std::make_shared<TestTarget_1>(i));
		for(int i = 0;i<3;i++)
			noti.push(std::make_shared<TestTarget_2>(i));
		
		std::unique_lock<std::mutex> lk(mtx);
		cv.wait(lk, [&](){return count >= 6;});
	}
	if(t.joinable())
		t.join();
}