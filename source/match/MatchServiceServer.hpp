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

/**
 * @brief gRPC Service Server for Match Server to accept from Battle Server.
 * 
 */
class MatchServiceServer final : public MatchToBattle::Service
{
	private:
		static constexpr int RESPONSE_TIME = 500;	//Timeout for Battle Server to transfer match.(ms)

		Status SessionStream(ServerContext*, ServerReaderWriter<MatchTransfer, Usage>*) override;
		DeMap<Seed_t, ServerReaderWriter<MatchTransfer, Usage>*, size_t> streams;

		//For Waiting result of Transfer
		int Recent_Success;
		std::mutex mtx;
		std::condition_variable cv;
	
	public:
		MatchServiceServer();
		/**
		 * @brief Send Match to Battle Server of least usage.(Least Connection Method of Load-Balancing)
		 * 
		 * @param lpid left player id
		 * @param rpid right player id
		 * @return Seed_t Battle Server's ID.
		 */
		Seed_t TransferMatch(const Account_ID_t &lpid, const Account_ID_t &rpid);
		std::vector<Seed_t> GetStreams();
		size_t GetUsage() const;
};