#pragma once
#include <memory>
#include <thread>
#include <functional>
#include <grpcpp/grpcpp.h>
#include "ServerService.grpc.pb.h"
#include "ThreadExceptHandler.hpp"
#include "StackTraceExcept.hpp"

using mylib::threads::ThreadExceptHandler;
using mylib::utils::StackTraceExcept;
using mylib::utils::ErrorCodeExcept;
using mylib::utils::ErrorCode;
using grpc::Channel;
using grpc::ClientReaderWriter;
using grpc::ClientContext;
using grpc::Status;
using ServerService::Usage;
using ServerService::MatchTransfer;
using ServerService::MatchToBattle;

/**
 * @brief gRPC Service Client for Battle Server to connect to Match Server.
 * 
 */
class BattleServiceClient : public ThreadExceptHandler
{
	private:
		using id_t = uint64_t;
		static constexpr int RECONNECT_TIME = 1000;	//ms

		Seed_t machine_id;
		bool isRunning;
		bool isConnected;
		std::unique_ptr<MatchToBattle::Stub> stub_;
		std::weak_ptr<ClientReaderWriter<Usage, MatchTransfer>> weak_stream;
		std::thread t;
		std::function<void(id_t, id_t)> callback;

		void ReceiveLoop();
	
	public:
		/**
		 * @brief Constructor of Battle Service Client.
		 * 
		 * @param addr address of Match Server
		 * @param port port of Match Server's Match Service
		 * @param mid Machine ID of itself
		 * @param p parent Thread Except Handler
		 */
		BattleServiceClient(const std::string &addr, const int &port, const Seed_t &mid, ThreadExceptHandler *p = nullptr);
		~BattleServiceClient();
		/**
		 * @brief is Stream still opened?
		 * 
		 * @return true 
		 * @return false 
		 */
		bool is_open() const;
		/**
		 * @brief Close Stream to Match Service.
		 * 
		 */
		void Close();
		/**
		 * @brief Set callback function for 'MatchTransfer()' from Match Service Server.
		 * 
		 * @param func 
		 */
		void SetCallback(const std::function<void(id_t, id_t)> &func);
		/**
		 * @brief let Match Server know it's usage.
		 * 
		 * @param usage the number of Game.
		 */
		void SetUsage(const size_t &usage);
};