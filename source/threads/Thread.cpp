#include "Thread.hpp"

namespace mylib
{
	namespace threads
	{
		Thread::Thread()
		: tc_thread(), tc_function(), MyThreadExceptInterface()
		{
			memset_vars();
		}

		Thread::Thread(std::function<void(tWorkerArg)> func, MyThreadExceptInterface* tei, bool justStart)
		: MyThreadExceptInterface(tei)
		{
			Thread();

			tc_function = func;

			if(justStart)
				start();
		}

		Thread::Thread(std::function<void(tWorkerArg)> func, bool justStart)
		: MyThreadExceptInterface()
		{
			Thread();

			tc_function = func;

			if(justStart)
				start();
		}

		Thread::~Thread()
		{
			if(tc_KillSwitch != nullptr)	(*tc_KillSwitch) = true;
			stop();
			if(tc_thread != nullptr && tc_thread->joinable())
				tc_thread->join();
		}

		Thread::Thread(Thread &&move) noexcept
		{
			(*this) = std::move(move);
		}

		Thread& Thread::operator=(Thread&& move) noexcept
		{
			tc_isRunning = std::move(move.tc_isRunning);
			tc_KillSwitch = std::move(move.tc_KillSwitch);
			tc_cv = std::move(move.tc_cv);
			tc_function = move.tc_function;

			tc_mutex = std::move(move.tc_mutex);
			tc_thread = std::move(move.tc_thread);

			return *this;
		}

		void Thread::memset_vars()	//Warning : Thread를 안정적으로 종료/이동 시키지 않은 상태로 사용하지 마시오.
		{
			tc_isRunning = std::make_shared<bool>(false);
			tc_KillSwitch = std::make_shared<bool>(false);
			tc_cv = std::make_shared<std::condition_variable>();

			tc_mutex = std::make_shared<std::mutex>();
		}

		bool Thread::isMemSet()
		{
			return tc_isRunning && tc_KillSwitch && tc_mutex;
		}

		bool Thread::start()
		{
			if(isRunning())
				return false;

			if(!isMemSet())
				memset_vars();
			
			(*tc_isRunning) = true;
			(*tc_KillSwitch) = false;

			tc_thread = std::make_shared<std::thread>(&Thread::Process, 
				std::forward<std::shared_ptr<bool>>(tc_isRunning),
				std::forward<std::shared_ptr<bool>>(tc_KillSwitch),
				std::forward<std::shared_ptr<std::condition_variable>>(tc_cv),
				std::forward<std::function<void(tWorkerArg)>>(tc_function),
				std::forward<MyThreadExceptInterface*>(this)
			);

			return true;
		}

		void Thread::stop()
		{
			if(!tc_thread)		//thread pointer가 null이면 초기화 이후 사용되지 않았거나(start를 한번도 안함) move된 객체이다.
				return;

			(*tc_KillSwitch) = true;
			std::unique_lock<std::mutex> lk(*tc_mutex);
			tc_cv->wait(lk, [&](){return !isRunning();});
			if(tc_thread->joinable())
			{
				tc_thread->join();
				tc_cv->notify_all();	//tc_cv->notify_one()으로 열린 thread이므로, 나머지 stop()에 묶인 thread들 열어서 sleep 빠져나오게 하기.
			}
		}

		bool Thread::SetFunction(std::function<void(tWorkerArg)> func)
		{
			if(isRunning())
				return false;

			tc_function = func;
			return true;
		}

		bool Thread::isRunning()
		{
			return (tc_isRunning == nullptr)?false:*tc_isRunning;
		}

		void Thread::Process(
			std::shared_ptr<bool> runswitch,
			std::shared_ptr<bool> killswitch,
			std::shared_ptr<std::condition_variable> cv,
			std::function<void(tWorkerArg)> function,
			MyThreadExceptInterface* tei
			)
		{
			std::exception_ptr exception;
			try
			{
				if(function)
					function(killswitch);
			}
			catch(const std::exception& e)
			{
				exception = std::make_exception_ptr(e);
			}
			(*runswitch) = false;
			cv->notify_one();	//stop()에 묶인 thread 중 1개만 열어서 tc_thread.join()을 수행하게 한다.

			if(exception)
				tei->ThrowThreadException(exception);
		}
	}
}