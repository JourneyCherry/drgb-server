#include "Pool.hpp"

namespace mylib{
namespace threads{

Pool::Pool(int max) : Max_Worker(max), isRunning(true)
{

}

Pool::~Pool()
{
	if(isRunning)
		Stop();
}

void Pool::Stop()
{
	isRunning = false;
	cv.notify_all();
	while(pool.size() > 0 || gc_queue.size() > 0)
	{
		ulock lk(mtx);
		cv.wait(lk, [&](){return !gc_queue.empty();});
		auto [ept, thrd] = gc_queue.front();
		gc_queue.pop();
		if(thrd->joinable())
			thrd->join();
	}
}

Pool::Key Pool::GetKey()
{
	static Key key = 0;
	Key start_key = key;
	while(pool.find(key) != pool.end())
	{
		key++;
		if(start_key == key)
			throw StackTraceExcept("Thread Pool Exceed.", __STACKINFO__);
	}

	return key++;
}

void Pool::Worker(Pool::Key key, Pool::Value value)
{
	std::exception_ptr eptr = nullptr;
	try
	{
		value->Work();
	}
	catch(const std::exception &e)
	{
		eptr = std::current_exception();
	}

	ulock lk(mtx);
	auto thrd = pool[key];
	pool.erase(key);
	gc_queue.push({{value, eptr, eptr == nullptr}, thrd});
	lk.unlock();

	cv.notify_one();
}

bool Pool::insert(Pool::Value value)
{
	if(!isRunning)
		return false;
	try
	{
		ulock lk(mtx);
		if(pool.size() >= Max_Worker)
			return false;
		Key newKey = GetKey();
		pool.insert({newKey, std::make_shared<std::thread>(std::bind(&Pool::Worker, this, newKey, value))});
	}
	catch(StackTraceExcept e)
	{
		e.stack(__STACKINFO__);
		throw e;
	}
	catch(const std::exception &e)
	{
		throw StackTraceExcept(e.what(), __STACKINFO__);
	}

	return true;
}

Pool::Value Pool::WaitForFinish()
{
	ulock lk(mtx);
	cv.wait(lk, [&](){return !isRunning || !gc_queue.empty();});
	if(gc_queue.empty())
		return nullptr;
	auto [ept, t] = gc_queue.front();
	gc_queue.pop();
	lk.unlock();

	if(t->joinable())
		t->join();

	if(!ept)
		std::rethrow_exception(ept.error());
	
	return *ept;
}

size_t Pool::size() const
{
	return pool.size();
}

}
}
