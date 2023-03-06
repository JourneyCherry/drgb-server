#include "Thread.hpp"

namespace mylib{
namespace threads{

Thread::Thread()
: ThreadExceptHandler()
{
	initMemory();
}

Thread::Thread(std::function<void()> func, ThreadExceptHandler* tei, bool justStart)
: ThreadExceptHandler(tei)
{
	initMemory();

	tc_function = func;

	if(justStart)
		start();
}

Thread::~Thread()
{
	join();
}

Thread::Thread(Thread &&move) noexcept
{
	(*this) = std::move(move);
}

Thread& Thread::operator=(Thread&& move) noexcept
{
	tc_isRunning = move.tc_isRunning;
	tc_cv = move.tc_cv;
	tc_function = move.tc_function;

	tc_mutex = move.tc_mutex;
	tc_thread = move.tc_thread;
	tc_tei = move.tc_tei;
	tc_tei->SetParent(this);

	move.initMemory();

	return *this;
}

void Thread::initMemory()
{
	tc_isRunning = std::make_shared<bool>(false);
	tc_cv = std::make_shared<std::condition_variable>();
	tc_function = nullptr;
	tc_tei = std::make_shared<ThreadExceptHandler>(this);

	tc_mutex = std::make_shared<std::mutex>();
	tc_thread = nullptr;
}

bool Thread::start()
{
	if(isRunning())
		return false;

	tc_thread = std::make_shared<std::thread>(&Thread::Process, 
		tc_isRunning,
		tc_cv,
		tc_function,
		tc_tei
	);

	return true;
}

void Thread::join()
{
	if(!tc_thread)		//thread pointer가 null이면 초기화 이후 사용되지 않았거나(start를 한번도 안함) move된 객체이다.
		return;

	std::unique_lock<std::mutex> lk(*tc_mutex);
	tc_cv->wait(lk, [&](){return !isRunning();});
	if(!tc_thread)
		return;
	if(tc_thread->joinable())
		tc_thread->join();
		
	tc_thread.reset();
	tc_cv->notify_all();	//tc_cv->notify_one()으로 열린 thread이므로, 나머지 stop()에 묶인 thread들 열어서 sleep 빠져나오게 하기.
}

bool Thread::SetFunction(std::function<void()> func)
{
	if(isRunning())
		return false;

	tc_function = func;
	return true;
}

bool Thread::isRunning()
{
	return *tc_isRunning;
}

void Thread::Process(
	std::shared_ptr<bool> isRunning,
	std::shared_ptr<std::condition_variable> cv,
	std::function<void()> function,
	std::shared_ptr<ThreadExceptHandler> tei
	)
{
	std::exception_ptr exception;
	(*isRunning) = true;
	try
	{
		if(function)
			function();
	}
	catch(...)
	{
		exception = std::current_exception();
	}
	(*isRunning) = false;
	cv->notify_one();

	if(exception && tei)
		tei->ThrowThreadException(exception);
}

void Thread::SetThreadName(const std::string &name)
{
#ifdef __DEBUG__
	pthread_setname_np(pthread_self(), name.c_str());
#endif
}

}
}