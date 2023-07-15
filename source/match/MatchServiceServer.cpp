#include "MatchServiceServer.hpp"

MatchServiceServer::MatchServiceServer() : ReceiveCount(0)
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
		size_t usage = msg.usage();
		//현재 stream이 저장된 데이터에 usage 저장
		streams.InsertLKeyValue(stream_id, usage);
		std::unique_lock lk(mtx);
		ReceiveCount++;
		lk.unlock();
		cv.notify_all();
	}

	//저장된 현재 stream 제거
	streams.EraseLKey(stream_id);
	return Status::OK;
}

Seed_t MatchServiceServer::TransferMatch(const Account_ID_t& lpid, const Account_ID_t& rpid)
{
	//저장된 stream 찾아서 stream->Write(send)
	auto vec = streams.GetAll();
	auto result = std::min_element(vec.begin(), vec.end(), [](auto lhs, auto rhs){ return (std::get<2>(lhs) < std::get<2>(rhs)); });
	if(result == vec.end())
		return -1;
		
	MatchTransfer send;
	send.mutable_lpid()->set_id(lpid);
	send.mutable_rpid()->set_id(rpid);

	if(!std::get<1>(*result)->Write(send))
		return -1;

	std::unique_lock lk(mtx);
	cv.wait_for(lk, std::chrono::milliseconds(RESPONSE_TIME), [this](){ return ReceiveCount > 0; });
	ReceiveCount = 0;
	lk.unlock();

	return std::get<0>(*result);
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