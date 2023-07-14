#include "ManagerServiceClient.hpp"

ManagerServiceClient::ManagerServiceClient(const std::string& addr, const int& port)
 : stub_(MgrToServer::NewStub(grpc::CreateChannel(addr + ":" + std::to_string(port), grpc::InsecureChannelCredentials())))
{}

Expected<size_t, ErrorCode> ManagerServiceClient::GetUsage()
{
	ClientContext context;
	Empty request;
	Usage reply;
	Status status = stub_->GetUsage(&context, request, &reply);
	if(!status.ok())
	{
		ErrorCode ec(status.error_code());
		ec.SetMessage(status.error_message());
		return ec;
	}

	return reply.usage();
}

Expected<bool, ErrorCode> ManagerServiceClient::CheckAccount(const Account_ID_t& id)
{
	ClientContext context;
	Account request;
	CheckResult reply;

	request.set_id(id);
	Status status = stub_->CheckAccount(&context, request, &reply);
	if(!status.ok())
	{
		ErrorCode ec(status.error_code());
		ec.SetMessage(status.error_message());
		return ec;
	}

	return reply.result();
}

Expected<size_t, ErrorCode> ManagerServiceClient::GetClientUsage()
{
	ClientContext context;
	Empty request;
	Usage reply;

	Status status = stub_->GetClientUsage(&context, request, &reply);
	if(!status.ok())
	{
		ErrorCode ec(status.error_code());
		ec.SetMessage(status.error_message());
		return ec;
	}

	return reply.usage();
}

Expected<std::map<std::string, size_t>, ErrorCode> ManagerServiceClient::GetConnectUsage()
{
	ClientContext context;
	Empty request;
	ConnectionUsage reply;
	Status status = stub_->GetConnectUsage(&context, request, &reply);
	if(!status.ok())
	{
		ErrorCode ec(status.error_code());
		ec.SetMessage(status.error_message());
		return ec;
	}

	std::map<std::string, size_t> connected;
	connected.insert(reply.serverconnected().begin(), reply.serverconnected().end());	//변경할 필요가 없으므로 굳이 mutable_*()을 호출할 필요가 없다.

	return connected;
}