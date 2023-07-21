#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include "MyWebsocketServer.hpp"
#include "ThreadExceptHandler.hpp"
#include "Logger.hpp"
#include "ConfigParser.hpp"
#include "ManagerServiceServer.hpp"
#include "MyRedis.hpp"
#include "MyPostgresPool.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using mylib::threads::ThreadExceptHandler;
using mylib::utils::ConfigParser;
using mylib::utils::Logger;
using mylib::utils::StackTraceExcept;

class MyServer : public ThreadExceptHandler
{
	protected:
		bool isRunning;
		static constexpr int MAX_CLIENTS = 5;		//최대 수용 가능 클라이언트 수. //TODO : 추후 확장 필요.
		static constexpr int TIME_AUTHENTICATE = 2000;	//인증(쿠키) 대기 시간.(ms)
		static constexpr int SESSION_TIMEOUT = 60000;	//세션 만료 시간.(ms)

	private:
		static constexpr int MAX_SOCKET = 2;
		int port_web;
		int port_tcp;
	
		std::thread ServiceThread;
		std::unique_ptr<Server> ServiceServer;
		ManagerServiceServer MgrService;

	protected:
		MyWebsocketServer web_server;

		ServerBuilder ServiceBuilder;

		MyPostgresPool dbpool;
		MyRedis redis;

	public:
		MyServer(int);
		virtual ~MyServer();
		void Start();
		void Stop();
		void Join();

	protected:
		virtual void Open() = 0;
		virtual void Close() = 0;
		virtual void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) = 0;

		//For Manager Service
		virtual size_t GetUsage();
		virtual bool CheckAccount(Account_ID_t);
		virtual size_t GetClientUsage();
		virtual std::map<std::string, size_t> GetConnectUsage();
};