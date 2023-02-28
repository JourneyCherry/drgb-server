#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include "Expected.hpp"

namespace mylib{
namespace utils{

class NotifyTarget
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
		std::queue<std::shared_ptr<NotifyTarget>> messages;

	public:
		Notifier() : isRunning(true){}
		~Notifier();
		void push(std::shared_ptr<NotifyTarget>);
		Expected<std::shared_ptr<NotifyTarget>> wait();
};

}
}