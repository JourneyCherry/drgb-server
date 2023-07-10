#include "BattleServiceClient.hpp"

BattleServiceClient::BattleServiceClient(const std::string& addr, const int& port, const Seed_t& mid, ThreadExceptHandler *p)
 : stub_(MatchToBattle::NewStub(grpc::CreateChannel(addr + ":" + std::to_string(port), grpc::InsecureChannelCredentials()))),
 isRunning(true),
 isConnected(false),
 machine_id(mid),
 ThreadExceptHandler(p)
{
	t = std::thread(std::bind(&BattleServiceClient::ReceiveLoop, this));
}

BattleServiceClient::~BattleServiceClient()
{
	if(isRunning)
		Close();
	if(t.joinable())
		t.join();
}

void BattleServiceClient::ReceiveLoop()
{
	while(isRunning)
	{
		ClientContext context;
		std::shared_ptr<ClientReaderWriter<Usage, MatchTransfer>> stream(stub_->SessionStream(&context));	//실제 연결시도는 여기서 이루어진다.
		weak_stream = stream;

		isConnected = true;
		SetUsage(machine_id);
		MatchTransfer received;
		while(stream->Read(&received))
			callback(received.lpid().id(), received.rpid().id());
		isConnected = false;

		Status status = stream->Finish();
		//연결이 끊어져도 즉시 재연결을 시도하므로, 예외를 보내지 않는다.
		if(!status.ok())
		{
			//단, 등록된 사유는 로그로 남겨둔다.
			switch(status.error_code())
			{
				case grpc::StatusCode::RESOURCE_EXHAUSTED:
				case grpc::StatusCode::ALREADY_EXISTS:
					{
						ErrorCode ec(status.error_code());
						ec.SetMessage(status.error_message());
						ThrowThreadException(std::make_exception_ptr(ErrorCodeExcept(ec, __STACKINFO__)));
					}
					break;
			}
		}

		if(!isRunning)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_TIME));
	}
}

bool BattleServiceClient::is_open() const
{
	return isConnected;
}

void BattleServiceClient::Close()
{
	isRunning = false;
	auto stream = weak_stream.lock();
	if(stream != nullptr)
		stream->WritesDone();
}

void BattleServiceClient::SetCallback(const std::function<void(id_t, id_t)>& func)
{
	callback = func;
}

void BattleServiceClient::SetUsage(const size_t& usage)
{
	auto stream = weak_stream.lock();
	if(stream == nullptr)
		return;

	Usage send;
	send.set_usage(usage);
	stream->Write(send);	//실패 여부 상관없음. 재연결 시도를 ReceiveLoop()에서 처리하기 때문.
}