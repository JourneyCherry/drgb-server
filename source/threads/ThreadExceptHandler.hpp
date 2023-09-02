#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace mylib{
namespace threads{

/**
 * @brief Handler to control exception thrown on child threads.
 * 
 */
class ThreadExceptHandler
{
	private:
		ThreadExceptHandler *parent;
		bool isGracefullyStop;
	protected:
		std::mutex Exception_Mutex;
		std::condition_variable Exception_CV;
		std::queue<std::exception_ptr> Exception_Exps;
		/**
		 * @brief Wait until one of child threads throw an exception.
		 * 
		 */
		void WaitForThreadException();
		/**
		 * @brief wake up threads calling and blocked by @ref WaitForThreadException().
		 * 
		 */
		void GracefulWakeUp();
	public:
		ThreadExceptHandler() : parent(nullptr), isGracefullyStop(false) {}
		/**
		 * @brief Constructor of ThreadExceptHandler with parent handler. All Exception signal will be passed to parent handler.
		 * 
		 * @param p parent handler.
		 */
		ThreadExceptHandler(ThreadExceptHandler* p) : parent(p), isGracefullyStop(false) {}
		/**
		 * @brief Set the parent Handler. All Exception signal will be passed to parent handler.
		 * 
		 * @param p parent handler.
		 */
		void SetParent(ThreadExceptHandler *p){parent = p;}
		/**
		 * @brief Throw std::current_exception() exception. It should be called on child threads.
		 * 
		 */
		void ThrowThreadException();	//자식 Thread의 try catch문의 catch 내부에서 호출되어야 함
		/**
		 * @brief Throw exception. It should be called on child threads
		 * 
		 * @param now_except exception pointer to throw.
		 * 
		 */
		void ThrowThreadException(std::exception_ptr now_except);
};

}
}