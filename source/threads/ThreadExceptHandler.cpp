#include "ThreadExceptHandler.hpp"

namespace mylib{
namespace threads{

void ThreadExceptHandler::ThrowThreadException()
{
	ThrowThreadException(std::current_exception());
}

void ThreadExceptHandler::ThrowThreadException(std::exception_ptr now_except)
{
	if(parent)
	{
		parent->ThrowThreadException(now_except);
		return;
	}
	std::unique_lock<std::mutex> lock(Exception_Mutex);
	Exception_Exps.push(now_except);
	lock.unlock();
	Exception_CV.notify_all();
}

void ThreadExceptHandler::WaitForThreadException()
{
	std::unique_lock<std::mutex> lock(Exception_Mutex);
	Exception_CV.wait(lock, [this]{ return (!Exception_Exps.empty() || isGracefullyStop); });
	if(Exception_Exps.empty())
		return;
	std::exception_ptr exp = Exception_Exps.front();
	Exception_Exps.pop();
	lock.unlock();
	std::rethrow_exception(exp);
}

void ThreadExceptHandler::GracefulWakeUp()
{
	isGracefullyStop = true;
	Exception_CV.notify_all();
}

}
}