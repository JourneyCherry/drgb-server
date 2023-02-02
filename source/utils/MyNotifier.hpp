#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include "MyExpected.hpp"

class MyNotifyTarget
{
	public:
		virtual int Type() = 0;
};

class MyNotifier
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		bool isRunning;
		std::mutex mtx;
		std::condition_variable cv;
		std::queue<std::shared_ptr<MyNotifyTarget>> messages;

	public:
		MyNotifier() : isRunning(true){}
		~MyNotifier();
		void push(std::shared_ptr<MyNotifyTarget>);
		MyExpected<std::shared_ptr<MyNotifyTarget>> wait();
};