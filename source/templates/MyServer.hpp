#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include "MyBytes.hpp"
#include "MyWebsocketServer.hpp"
#include "MyWebsocketClient.hpp"
#include "MyTCPServer.hpp"
#include "MyTCPClient.hpp"
#include "Thread.hpp"

class MyServer : public MyThreadExceptInterface
{
	protected:
		bool isRunning;
		static constexpr int MAX_CLIENTS = 5;		//최대 수용 가능 클라이언트 수. //TODO : 추후 확장 필요.

	private:
		using Thread = mylib::threads::Thread;

		static constexpr int MAX_SOCKET = 2;
		std::array<std::shared_ptr<MyServerSocket>, MAX_SOCKET> sockets;
		std::array<Thread, MAX_SOCKET> acceptors;

		using ulock = std::unique_lock<std::mutex>;
		std::mutex m_accepts;
		std::queue<std::shared_ptr<MyClientSocket>> q_accepts;
		std::condition_variable cv_workers;
		std::vector<std::shared_ptr<Thread>> v_workers;

	public:
		MyServer(int, int);
		virtual ~MyServer();
		void Start();
		void Stop();
		void Join();

	protected:
		virtual void Open() = 0;
		virtual void Close() = 0;
		virtual void ClientProcess(std::shared_ptr<MyClientSocket>) = 0;
	
	private:
		void Accept(std::shared_ptr<bool>, std::shared_ptr<MyServerSocket>);
		void Work(std::shared_ptr<bool>);
};