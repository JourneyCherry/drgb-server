#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include "Expected.hpp"

namespace mylib{
namespace utils{

class MyNotifyTarget
{
	public:
		virtual int Type() = 0;
};

class Notifier
{
	private:
		using ulock = std::unique_lock<std::mutex>;

		bool isRunning;
		std::mutex mtx;
		std::condition_variable cv;
		std::queue<std::shared_ptr<MyNotifyTarget>> messages;

	public:
		Notifier() : isRunning(true){}
		~Notifier();
		void push(std::shared_ptr<MyNotifyTarget>);
		Expected<std::shared_ptr<MyNotifyTarget>> wait();
};

}
}