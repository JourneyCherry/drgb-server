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

/**
 * @brief gRPC Client for Manager. Each Method will send request and receive answer packet. If failed, return ErrorCode instead.
 * 
 */
class ManagerServiceClient
{
	private:
		std::unique_ptr<MgrToServer::Stub> stub_;

	public:
		/**
		 * @brief Constructor of Manager Service Client. It'll try to connect the server.
		 * 
		 * @param addr address of the server
		 * @param port port of the server
		 */
		ManagerServiceClient(const std::string &addr, const int &port);
		~ManagerServiceClient() = default;

		Expected<size_t, ErrorCode> GetUsage();
		Expected<bool, ErrorCode> CheckAccount(const Account_ID_t &id);
		Expected<size_t, ErrorCode> GetClientUsage();
		Expected<std::map<std::string, size_t>, ErrorCode> GetConnectUsage();
};