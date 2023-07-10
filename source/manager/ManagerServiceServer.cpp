#include "ManagerServiceServer.hpp"

ManagerServiceServer::ManagerServiceServer(
	const GetUsage_t& h_gu, 
	const CheckAccount_t& h_ca, 
	const GetClientUsage_t& h_gcu, 
	const GetConnectUsage_t& h_gcnu
	) : 
	handler_GetUsage(h_gu), 
	handler_CheckAccount(h_ca),
	handler_GetClientUsage(h_gcu),
	handler_GetConnectUsage(h_gcnu)
{}

Status ManagerServiceServer::GetUsage(ServerContext* context, const Empty* request, Usage* reply)
{
	reply->set_usage(handler_GetUsage());
	return Status::OK;
}

Status ManagerServiceServer::CheckAccount(ServerContext* context, const Account* request, CheckResult* reply)
{
	reply->set_result(handler_CheckAccount(request->id()));
	return Status::OK;	
}

Status ManagerServiceServer::GetClientUsage(ServerContext* context, const Empty* request, ClientUsage* reply)
{
	auto pair = handler_GetClientUsage();
	reply->mutable_tcp()->set_usage(pair.first);
	reply->mutable_web()->set_usage(pair.second);
	return Status::OK;
}

Status ManagerServiceServer::GetConnectUsage(ServerContext* context, const Empty* request, ConnectionUsage* reply)
{
	auto connected = handler_GetConnectUsage();
	reply->mutable_serverconnected()->insert(connected.begin(), connected.end());

	return Status::OK;
}
