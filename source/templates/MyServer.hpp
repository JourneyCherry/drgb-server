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

/**
 * @brief Dedicated Server Template. It contains Common Process for every type of server.
 * 
 */
class MyServer : public ThreadExceptHandler
{
	protected:
		bool isRunning;
		static constexpr int TIME_AUTHENTICATE = 2000;	//인증(쿠키) 대기 시간.(ms)
		static constexpr int SESSION_TIMEOUT = 60000;	//세션 만료 시간.(ms) 이 절반 시간 간격으로 HeartBeat패킷을 보내고, 체크한다.
		static constexpr int SESSION_TTL = 4;			//세션 만료 횟수. HeartBeat패킷을 받았을 때, TTL 초기화 값. 매 HeartBeat체크마다 1씩 감소하고, 0이 되면 세션연결을 종료한다.

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
		/**
		 * @brief Constructor of Server Template.
		 * 
		 * @param port port number of Service to communicate with other servers.
		 */
		MyServer(int port);
		virtual ~MyServer();
		/**
		 * @brief Start Server. The Server runs until 'Stop()' is called.
		 * 
		 */
		void Start();
		/**
		 * @brief Stop Server. It wakes up every 'Join()'ed thread.
		 * 
		 */
		void Stop();
		/**
		 * @brief Wait until the Server stops. 
		 * @throw std::exception every exception that occurs on running.
		 * 
		 */
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