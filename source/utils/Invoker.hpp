#pragma once
#include <condition_variable>
#include <functional>

namespace mylib{
namespace utils{

//특정 변수의 값이 변경되었을 때, 원하는 값이 될 때 까지 대기할 때 사용.
template <typename T>
class Invoker
{
	private:
		using lock = std::unique_lock<std::mutex>;
	private:
		bool killswitch;
		std::mutex m;
		std::condition_variable cv;
		T data;

	public:
		Invoker()
		{
			use();
			//초기화 과정이 필요하지만, enum같이 초기화용 생성자가 없는 타입도 있으므로, default initialization이 되도록 냅둔다.
		}
		Invoker(const T& value)
		{
			use();
			data = value;
		}
		~Invoker()
		{
			release();
		}
		void use()
		{
			killswitch = false;
		}
		void release()
		{
			killswitch = true;
			cv.notify_all();
		}
		Invoker& operator=(const T& value)
		{
			lock lk(m);
			data = value;

			cv.notify_all();

			return *this;
		}
		bool operator==(const T& value)
		{
			return data == value;
		}
		bool operator!=(const T& value)
		{
			return !operator==(value);
		}
		bool wait(const T& target)
		{
			return wait([target](const T& value){return target == value;});
		}
		bool wait(std::function<bool(const T&)> cmp)
		{
			lock lk(m);
			cv.wait(lk, [this, cmp](){return cmp(this->data)||this->killswitch;});
			return !killswitch;
		}
		operator T()	//Implicit Type Casting
		{
			return data;	//T가 bool일 경우, if문 내에서 정상적으로 bool로 캐스팅 됨.
		}
		T get()
		{
			return data;
		}
};

}
}