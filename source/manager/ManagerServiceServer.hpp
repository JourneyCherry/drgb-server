#pragma once
#include <utility>
#include <map>
#include <string>
#include <functional>
#include <grpcpp/grpcpp.h>
#include "MyTypes.hpp"
#include "ServerService.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;
using ServerService::Usage;
using ServerService::Account;
using ServerService::CheckResult;
using ServerService::ConnectionUsage;
using google::protobuf::Empty;
using ServerService::MgrToServer;

class ManagerServiceServer : public MgrToServer::Service
{
	private:
		using GetUsage_t = std::function<size_t()>;
		using CheckAccount_t = std::function<bool(Account_ID_t)>;
		using GetClientUsage_t = std::function<size_t()>;
		using GetConnectUsage_t = std::function<std::map<std::string, size_t>()>;
		GetUsage_t handler_GetUsage;
		CheckAccount_t handler_CheckAccount;
		GetClientUsage_t handler_GetClientUsage;
		GetConnectUsage_t handler_GetConnectUsage;
	private:
		Status GetUsage(ServerContext*, const Empty*, Usage*) override;
		Status CheckAccount(ServerContext*, const Account*, CheckResult*) override;
		Status GetClientUsage(ServerContext*, const Empty*, Usage*) override;
		Status GetConnectUsage(ServerContext*, const Empty*, ConnectionUsage*) override;
	public:
		ManagerServiceServer(const GetUsage_t&, const CheckAccount_t&, const GetClientUsage_t&, const GetConnectUsage_t&);
		~ManagerServiceServer() = default;
};