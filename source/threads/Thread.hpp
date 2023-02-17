#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "MyExcepts.hpp"

namespace mylib
{
	namespace threads
	{
		class Thread : public MyThreadExceptInterface
		{
			private:
				using tWorkerArg = std::shared_ptr<bool>;

				std::shared_ptr<bool> tc_isRunning;
				std::shared_ptr<bool> tc_KillSwitch;
				std::shared_ptr<std::condition_variable> tc_cv;
				std::function<void(tWorkerArg)> tc_function;
				std::shared_ptr<MyThreadExceptInterface> tc_tei;

				std::shared_ptr<std::mutex> tc_mutex;
				std::shared_ptr<std::thread> tc_thread;

				static void Process(
					std::shared_ptr<bool>, 						// Run Switch
					std::shared_ptr<bool>, 						// Kill Switch
					std::shared_ptr<std::condition_variable>, 	// Condition Variable for Exit
					std::function<void(tWorkerArg)>, 			// Work Function
					std::shared_ptr<MyThreadExceptInterface>	// Exception Pointer
				);

				void memset_vars();
				bool isMemSet();

			public:
				Thread();
				Thread(std::function<void(tWorkerArg)>, MyThreadExceptInterface* = nullptr, bool = true);
				Thread(const Thread&) = delete;
				Thread(Thread&&) noexcept;
				~Thread();
				Thread& operator=(const Thread&) = delete;
				Thread& operator=(Thread&&) noexcept;

				bool start();
				void stop();	//기본적으로 JoinUnitilStop()이다.

				void SetThreadExcept(MyThreadExceptInterface*);
				bool SetFunction(std::function<void(tWorkerArg)>);

				bool isRunning();
		};
	}
}