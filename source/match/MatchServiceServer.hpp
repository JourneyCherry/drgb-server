#pragma once
#include <memory>
#include <thread>
#include <functional>
#include <limits>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include "DeMap.hpp"
#include "ServerService.grpc.pb.h"
#include "MyTypes.hpp"

using mylib::utils::DeMap;
using grpc::Channel;
using grpc::ServerReaderWriter;
using grpc::Status;
using grpc::ServerContext;
using ServerService::Usage;
using ServerService::Account;
using ServerService::MatchTransfer;
using ServerService::MatchToBattle;

class MatchServiceServer final : public MatchToBattle::Service
{
	private:
		static constexpr int RESPONSE_TIME = 500;	//ms

		Status SessionStream(ServerContext*, ServerReaderWriter<MatchTransfer, Usage>*) override;
		DeMap<Seed_t, ServerReaderWriter<MatchTransfer, Usage>*, size_t> streams;

		//For Waiting result of Transfer
		unsigned int ReceiveCount;
		std::mutex mtx;
		std::condition_variable cv;
	
	public:
		MatchServiceServer();
		Seed_t TransferMatch(const Account_ID_t&, const Account_ID_t&);
		std::vector<Seed_t> GetStreams();
		size_t GetUsage() const;
};