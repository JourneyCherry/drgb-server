#include "MyExcepts.hpp"

const std::string MyExcepts::CR = "\r\n";
const std::string MyExcepts::TAB = "\t";

MyExcepts::MyExcepts(std::string content, std::string file, std::string func, int line)
{
	stacktrace = "[MyException] " + content + CR + "[Stack Trace]" + CR;
	stack(file, func, line);
}

MyExcepts::MyExcepts(std::exception e, std::string file, std::string func, int line)
{
	stacktrace = "[Exception] " + std::string(e.what()) + CR + "[Stack Trace]" + CR;
	stack(file, func, line);
}

void MyExcepts::stack(std::string file, std::string func, int line)
{
	stacktrace += TAB + func + "(" + file + ":" + std::to_string(line) + ")" + CR;
}

const char * MyExcepts::what() const noexcept
{
	return stacktrace.c_str();
}

void MyExcepts::log()
{
	MyLogger::log(stacktrace, MyLogger::LogType::error);
}

void MyThreadExceptInterface::ThrowThreadException()
{
	ThrowThreadException(std::current_exception());
}

void MyThreadExceptInterface::ThrowThreadException(const std::exception &e)
{
	ThrowThreadException(std::make_exception_ptr(e));
}

void MyThreadExceptInterface::ThrowThreadException(std::exception_ptr now_except)
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

void MyThreadExceptInterface::WaitForThreadException()
{
	std::unique_lock<std::mutex> lock(Exception_Mutex);
	isGracefullyStop = false;
	Exception_CV.wait(lock, [this]{ return !Exception_Exps.empty(); });
	if(isGracefullyStop)
	{
		if(!Exception_Exps.empty())
		{
			std::exception_ptr exp = Exception_Exps.front();
			Exception_Exps.pop();
			std::rethrow_exception(exp);
		}
		return;
	}
	std::exception_ptr exp = Exception_Exps.front();
	Exception_Exps.pop();
	lock.unlock();
	std::rethrow_exception(exp);
}

void MyThreadExceptInterface::GracefulWakeUp()
{
	isGracefullyStop = true;
	Exception_CV.notify_all();
}