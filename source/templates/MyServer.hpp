#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include "MyWebsocketServer.hpp"
#include "MyTCPServer.hpp"
#include "Thread.hpp"
#include "Logger.hpp"
#include "FixedPool.hpp"

using mylib::threads::ThreadExceptHandler;
using mylib::threads::Thread;
using mylib::threads::FixedPool;
using mylib::utils::Logger;
using mylib::utils::StackTraceExcept;

class MyServer : public ThreadExceptHandler
{
	protected:
		bool isRunning;
		static constexpr int MAX_CLIENTS = 5;		//최대 수용 가능 클라이언트 수. //TODO : 추후 확장 필요.
		static constexpr int TIME_AUTHENTICATE = 2000;	//인증(쿠키) 대기 시간.(ms)

	private:
		static constexpr int MAX_SOCKET = 2;
		int port_web;
		int port_tcp;
	
	protected:
		MyTCPServer tcp_server;
		MyWebsocketServer web_server;

	public:
		MyServer(int, int);
		virtual ~MyServer();
		void Start();
		void Stop();
		void Join();

	protected:
		virtual void Open() = 0;
		virtual void Close() = 0;
		virtual void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) = 0;
};