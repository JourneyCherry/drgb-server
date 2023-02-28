#include "Notifier.hpp"

namespace mylib{
namespace utils{

Notifier::~Notifier()
{
	isRunning = false;
	cv.notify_all();
}

void Notifier::push(std::shared_ptr<MyNotifyTarget> msg)
{
	if(!isRunning)
		return;
	ulock lk(mtx);
	messages.push(msg);
	lk.unlock();
	cv.notify_all();
}

Expected<std::shared_ptr<MyNotifyTarget>> Notifier::wait()
{
	ulock lk(mtx);
	cv.wait(lk, [&](){return !isRunning || !messages.empty();});
	if(!isRunning)
		return {};
	std::shared_ptr<MyNotifyTarget> msg = messages.front();
	messages.pop();
	lk.unlock();

	return msg;
}

}
}