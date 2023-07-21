#include <gtest/gtest.h>
#include <thread>
#include "ThreadExceptHandler.hpp"

using mylib::threads::ThreadExceptHandler;

class Waiter : public ThreadExceptHandler
{
	public:
		void Wait()
		{
			WaitForThreadException();
		}
		void Wake()
		{
			GracefulWakeUp();
		}
};

TEST(ThreadExceptHandlerTest, ExceptionTest)
{
	Waiter waiter;
	try
	{
		throw std::runtime_error("Test Exception");
	}
	catch(...)
	{
		waiter.ThrowThreadException(std::current_exception());
	}
	ASSERT_ANY_THROW(waiter.Wait());
}

TEST(ThreadExceptHandlerTest, WakeUpTest)
{
	Waiter waiter;
	try
	{
		throw std::runtime_error("Test Exception");
	}
	catch(...)
	{
		waiter.ThrowThreadException(std::current_exception());
	}
	waiter.Wake();
	ASSERT_ANY_THROW(waiter.Wait());
	ASSERT_NO_THROW(waiter.Wait());
}

TEST(ThreadExceptHandler, ThreadTest)
{
	Waiter waiter;
	std::thread t([&waiter]()
	{
		for(int i = 0;i<5;i++)
		{
			try
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if(i%2 == 0)
					throw std::runtime_error(std::to_string(i) + "th iteration");
			}
			catch(...)
			{
				waiter.ThrowThreadException(std::current_exception());
			}
		}
		waiter.Wake();
	});
	while(true)
	{
		try
		{
			waiter.Wait();
			break;
		}
		catch(const std::exception& e)
		{
			std::cout << "Exception : " << e.what() << std::endl;
		}
	}
	if(t.joinable())
		t.join();
}

TEST(ThreadExceptHandler, ParentTest)
{
	Waiter grandparent;
	Waiter parent;
	Waiter child;
	Waiter grandchild;
	grandchild.SetParent(&child);
	child.SetParent(&parent);
	parent.SetParent(&grandparent);

	parent.ThrowThreadException(std::make_exception_ptr(std::runtime_error("This is From Parent")));
	child.ThrowThreadException(std::make_exception_ptr(std::runtime_error("This is From Child")));
	grandparent.ThrowThreadException(std::make_exception_ptr(std::runtime_error("This is From Grand Parent")));
	grandchild.ThrowThreadException(std::make_exception_ptr(std::runtime_error("This is From Grand Child")));

	grandchild.Wake();
	ASSERT_NO_THROW(grandchild.Wait());
	child.Wake();
	ASSERT_NO_THROW(child.Wait());
	parent.Wake();
	ASSERT_NO_THROW(parent.Wait());
	grandparent.Wake();
	while(true)
	{
		try
		{
			grandparent.Wait();
			break;
		}
		catch(const std::exception& e)
		{
			std::cout << "Exception : " << e.what() << std::endl;
		}
	}
}