#pragma once
#include <memory>
#include <thread>
#include <functional>
#include <grpcpp/grpcpp.h>
#include <limits>
#include "DeMap.hpp"
#include "ServerService.grpc.pb.h"
#include "MyCodes.hpp"

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
		using id_t = uint64_t;
		Status SessionStream(ServerContext*, ServerReaderWriter<MatchTransfer, Usage>*) override;
		DeMap<Seed_t, ServerReaderWriter<MatchTransfer, Usage>*, size_t> streams;
	
	public:
		Seed_t TransferMatch(const id_t&, const id_t&);
		std::vector<Seed_t> GetStreams();
		size_t GetUsage() const;
};