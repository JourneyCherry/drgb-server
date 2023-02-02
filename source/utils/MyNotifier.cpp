#include "MyNotifier.hpp"

MyNotifier::~MyNotifier()
{
	isRunning = false;
	cv.notify_all();
}

void MyNotifier::push(std::shared_ptr<MyNotifyTarget> msg)
{
	if(!isRunning)
		return;
	ulock lk(mtx);
	messages.push(msg);
	lk.unlock();
	cv.notify_all();
}

MyExpected<std::shared_ptr<MyNotifyTarget>> MyNotifier::wait()
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