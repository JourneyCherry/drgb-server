#pragma once
#include <stdexcept>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include "MyLogger.hpp"

#define __STACKINFO__	__FILE__, __FUNCTION__, __LINE__

class MyExcepts : public std::exception
{
	private:
		static const std::string CR;
		static const std::string TAB;
		std::string stacktrace;
	public:
		MyExcepts(std::string, std::string, std::string, int);
		MyExcepts(std::exception, std::string, std::string, int);
		void stack(std::string, std::string, int);
		const char * what() const noexcept override;
		void log();
};

class MyThreadExceptInterface
{
	private:
		MyThreadExceptInterface *parent;
		bool isGracefullyStop;
	protected:
		std::mutex Exception_Mutex;
		std::condition_variable Exception_CV;
		std::queue<std::exception_ptr> Exception_Exps;
		void WaitForThreadException();
		void GracefulWakeUp();
	public:
		MyThreadExceptInterface() : parent(nullptr), isGracefullyStop(false) {}
		MyThreadExceptInterface(MyThreadExceptInterface* p) : parent(p), isGracefullyStop(false) {}
		void SetParent(MyThreadExceptInterface *p){parent = p;}
		void ThrowThreadException();	//자식 Thread의 try catch문의 catch 내부에서 호출되어야 함
		void ThrowThreadException(std::exception_ptr);
		void ThrowThreadException(const std::exception &);
};