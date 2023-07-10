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
		BattleServiceClient(const std::string&, const int&, const Seed_t&, ThreadExceptHandler* = nullptr);
		~BattleServiceClient();
		bool is_open() const;
		void Close();
		void SetCallback(const std::function<void(id_t, id_t)>&);
		void SetUsage(const size_t&);
};