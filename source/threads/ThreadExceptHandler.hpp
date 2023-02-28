#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace mylib{
namespace threads{

class ThreadExceptHandler
{
	private:
		ThreadExceptHandler *parent;
		bool isGracefullyStop;
	protected:
		std::mutex Exception_Mutex;
		std::condition_variable Exception_CV;
		std::queue<std::exception_ptr> Exception_Exps;
		void WaitForThreadException();
		void GracefulWakeUp();
	public:
		ThreadExceptHandler() : parent(nullptr), isGracefullyStop(false) {}
		ThreadExceptHandler(ThreadExceptHandler* p) : parent(p), isGracefullyStop(false) {}
		void SetParent(ThreadExceptHandler *p){parent = p;}
		void ThrowThreadException();	//자식 Thread의 try catch문의 catch 내부에서 호출되어야 함
		void ThrowThreadException(std::exception_ptr);
};

}
}