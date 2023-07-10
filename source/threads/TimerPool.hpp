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
		TimerPool(const size_t&, ThreadExceptHandler*);
		~TimerPool();

		void Stop();
		std::shared_ptr<boost::asio::steady_timer> GetTimer();
		void ReleaseTimer(const std::shared_ptr<boost::asio::steady_timer>&);
		size_t Size() const;
		bool is_open() const;
};

}
}