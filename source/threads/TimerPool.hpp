#pragma once
#include <set>
#include <memory>
#include <mutex>
#include <functional>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include "ThreadExceptHandler.hpp"

using mylib::threads::ThreadExceptHandler;

namespace mylib{
namespace threads{

/**
 * @brief Timer Pool managing timers running on distinct thread
 * 
 */
class TimerPool : public ThreadExceptHandler
{
	private:
		bool isRunning;
		boost::asio::io_context ioc;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
		boost::asio::thread_pool ThreadPool;

		std::mutex timer_mtx;
		std::set<std::shared_ptr<boost::asio::steady_timer>> TimerSet;

	public:
		/**
		 * @brief Constructor of TimerPool object.
		 * 
		 * @param thread_num the number of distinct thread.
		 * @param p parent ThreadExceptHandler
		 */
		TimerPool(const size_t &thread_num, ThreadExceptHandler *p);
		~TimerPool();
		/**
		 * @brief Stop All Timer and threads.
		 * 
		 */
		void Stop();
		/**
		 * @brief Get a new timer from pool.
		 * 
		 * @return std::shared_ptr<boost::asio::steady_timer> new timer object's pointer. It should be returned after usage.
		 */
		std::shared_ptr<boost::asio::steady_timer> GetTimer();
		/**
		 * @brief Return timer that no needs to use.
		 * 
		 * @param timer timer pointer.
		 */
		void ReleaseTimer(const std::shared_ptr<boost::asio::steady_timer> &timer);
		/**
		 * @brief Get the number of created timer.
		 * 
		 * @return size_t the number of created timer.
		 */
		size_t Size() const;
		/**
		 * @brief Check if the Pool is opened or closed. It doesn't guarantee all timers are stopped.
		 * 
		 * @return true if the pool and timers are running.
		 * @return false if @ref Stop() is called.
		 */
		bool is_open() const;
};

}
}