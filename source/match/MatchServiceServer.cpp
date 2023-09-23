#include "MatchServiceServer.hpp"

MatchServiceServer::MatchServiceServer() : Recent_Success(-1)
{

}

Status MatchServiceServer::SessionStream(ServerContext *context, ServerReaderWriter<MatchTransfer, Usage>* stream)
{
	if(streams.Size() >= std::numeric_limits<Seed_t>::max() - 1)	//이미 수용량 초과일 경우
		return Status(grpc::StatusCode::RESOURCE_EXHAUSTED, "Stream Resource of Server Exceeds");

	//들어온 Stream의 ID 확인
	Usage machine_id_notification;
	if(!stream->Read(&machine_id_notification))
		return Status::CANCELLED;
	Seed_t stream_id = machine_id_notification.usage();
	if(streams.FindLKey(stream_id))
		return Status(grpc::StatusCode::ALREADY_EXISTS, "Stream is already exist by another machine.");

	//현재 stream 저장
	streams.Insert(stream_id, stream, 0);

	Usage msg;
	while(stream->Read(&msg))
	{
		std::unique_lock lk(mtx);
		size_t usage = msg.usage();
		Recent_Success = usage;
		bool connectivity = stream->Read(&msg);

		if(connectivity)
		{
			usage = msg.usage();
			//현재 stream이 저장된 데이터에 usage 저장
			streams.InsertLKeyValue(stream_id, usage);
		}
		else
			Recent_Success = 0;
		lk.unlock();
		cv.notify_all();

		if(!connectivity)
			break;
	}

	//저장된 현재 stream 제거
	streams.EraseLKey(stream_id);
	return Status::OK;
}

Seed_t MatchServiceServer::TransferMatch(const Account_ID_t& lpid, const Account_ID_t& rpid)
{
	//저장된 stream 찾아서 stream->Write(send)
	auto vec = streams.GetAll();

	//Transfer information
	MatchTransfer send;
	send.mutable_lpid()->set_id(lpid);
	send.mutable_rpid()->set_id(rpid);

	auto result = std::min_element(vec.begin(), vec.end(), [](auto lhs, auto rhs){ return (std::get<2>(lhs) < std::get<2>(rhs)); });
	while(result != vec.end())
	{
		Recent_Success = -1;
		if(std::get<1>(*result)->Write(send))	//Transfer Success
		{
			std::unique_lock lk(mtx);
			bool received = cv.wait_for(lk, std::chrono::milliseconds(RESPONSE_TIME), [this](){ return Recent_Success > 0; });	//Wait for Usage Response.
			lk.unlock();

			if(Recent_Success >= 1)			//Response Success
				return std::get<0>(*result);
		}

		vec.erase(result);
		result = std::min_element(vec.begin(), vec.end(), [](auto lhs, auto rhs){ return (std::get<2>(lhs) < std::get<2>(rhs)); });
	}

	return -1;
}

std::vector<Seed_t> MatchServiceServer::GetStreams()
{
	std::vector<Seed_t> result;
	auto stream = streams.GetAll();
	for(auto &[key, session, usage] : stream)
		result.push_back(key);
	return result;
}

size_t MatchServiceServer::GetUsage() const
{
	return streams.Size();
}