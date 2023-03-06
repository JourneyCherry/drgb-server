#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include "ThreadExceptHandler.hpp"

namespace mylib{
namespace threads{

class Thread : public ThreadExceptHandler
{
	private:

		std::shared_ptr<bool> tc_isRunning;
		std::shared_ptr<std::condition_variable> tc_cv;
		std::function<void()> tc_function;
		std::shared_ptr<ThreadExceptHandler> tc_tei;

		std::shared_ptr<std::mutex> tc_mutex;
		std::shared_ptr<std::thread> tc_thread;

		static void Process(
			std::shared_ptr<bool>, 						// Run Switch
			std::shared_ptr<std::condition_variable>, 	// Condition Variable for Exit
			std::function<void()>, 			// Work Function
			std::shared_ptr<ThreadExceptHandler>	// Exception Pointer
		);

		void initMemory();

	public:
		Thread();
		Thread(std::function<void()>, ThreadExceptHandler* = nullptr, bool = true);
		Thread(const Thread&) = delete;
		Thread(Thread&&) noexcept;
		~Thread();
		Thread& operator=(const Thread&) = delete;
		Thread& operator=(Thread&&) noexcept;

		bool start();
		void join();	//기본적으로 JoinUnitilStop()이다.

		void SetThreadExcept(ThreadExceptHandler*);
		bool SetFunction(std::function<void()>);

		bool isRunning();

		static void SetThreadName(const std::string&);
};

}
}