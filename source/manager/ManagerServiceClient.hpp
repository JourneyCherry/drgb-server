#pragma once
#include <grpcpp/grpcpp.h>
#include "MyTypes.hpp"
#include "StackTraceExcept.hpp"
#include "Expected.hpp"
#include "ServerService.grpc.pb.h"

using mylib::utils::Expected;
using mylib::utils::ErrorCode;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;
using ServerService::Usage;
using ServerService::Account;
using ServerService::CheckResult;
using ServerService::ConnectionUsage;
using ServerService::MgrToServer;

class ManagerServiceClient
{
	private:
		std::unique_ptr<MgrToServer::Stub> stub_;

	public:
		ManagerServiceClient(const std::string&, const int&);
		~ManagerServiceClient() = default;

		Expected<size_t, ErrorCode> GetUsage();
		Expected<bool, ErrorCode> CheckAccount(const Account_ID_t&);
		Expected<size_t, ErrorCode> GetClientUsage();
		Expected<std::map<std::string, size_t>, ErrorCode> GetConnectUsage();
};