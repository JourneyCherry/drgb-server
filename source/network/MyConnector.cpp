#include "MyConnector.hpp"

MyConnector::MyConnector(boost::asio::ip::tcp::socket _socket) : MyTCPClient(std::move(_socket))
{
}

MyConnector::~MyConnector()
{
	Close();
}

void MyConnector::StartInquiry(std::function<ByteQueue(ByteQueue)> inquiry)
{
	inquiry_process = inquiry;

	StartRecv(std::bind(&MyConnector::RecvHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void MyConnector::Connectee(std::function<bool(std::shared_ptr<MyClientSocket>, Expected<std::string, ErrorCode>)> callback)
{
	auto self_ptr = shared_from_this();
	KeyExchange([this, self_ptr, callback](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
	{
		if(!ke_ec.isSuccessed())
		{
			ke_socket->Close();
			return;
		}

		DoRecv([this, self_ptr, callback](boost::system::error_code error_code, size_t bytes_written)
		{
			if(error_code.failed() || bytes_written <= 0)
			{
				callback(self_ptr, ErrorCode(ERR_CONNECTION_CLOSED));
				return;
			}
			this->GetRecv(bytes_written);

			std::string remote_keyword = "";
			auto auth = this->Recv();
			if(auth)
				remote_keyword = auth->popstr();

			if(callback(self_ptr, remote_keyword))
			{
				this->keyword = remote_keyword;
				m_authorized = true;
			}
		});
	});
}

void MyConnector::Connector(std::string local_keyword, std::function<void(std::shared_ptr<MyClientSocket>, Expected<std::string, ErrorCode>)> callback)
{
	auto self_ptr = shared_from_this();
	KeyExchange([this, local_keyword, self_ptr, callback](std::shared_ptr<MyClientSocket> ke_socket, ErrorCode ke_ec)
	{
		if(!ke_ec.isSuccessed())
		{
			ke_socket->Close();
			return;
		}

		ke_socket->Send(ByteQueue::Create<std::string>(local_keyword));
		DoRecv([this, self_ptr, callback](boost::system::error_code error_code, size_t bytes_written)
		{
			if(error_code.failed() || bytes_written <= 0)
			{
				callback(self_ptr, ErrorCode(ERR_CONNECTION_CLOSED));
				return;
			}
			this->GetRecv(bytes_written);

			Expected<std::string, ErrorCode> remote_keyword(SUCCESS);
			auto auth = this->Recv();
			if(auth)
			{
				byte header = auth->pop<byte>();
				if(header == SUCCESS)
				{
					this->keyword = auth->popstr();
					remote_keyword = Expected<std::string, ErrorCode>(keyword);
					m_authorized = true;
				}
				else
					remote_keyword = Expected<std::string, ErrorCode>(ErrorCode(header));
			}
			else
				remote_keyword = Expected<std::string, ErrorCode>(ErrorCode(ERR_CONNECTION_CLOSED));

			callback(self_ptr, remote_keyword);
		});
	});
}

void MyConnector::RecvHandler(std::shared_ptr<MyClientSocket> socket_self, ByteQueue result, ErrorCode ec)
{
	if(!ec)
	{
		socket_self->Close();
		return;
	}

	try
	{
		byte header = result.pop<byte>();
		unsigned int id = result.pop<unsigned int>();
		ByteQueue msg = result.split();
		
		switch(header)
		{
			case INQ_REQUEST:
				{
					ByteQueue answer = inquiry_process(msg);
					answer.push_head<unsigned int>(id);
					answer.push_head<byte>(INQ_ANSWER);
					ErrorCodeExcept::ThrowOnFail(Send(answer), __STACKINFO__);	//성공, 실패는 관여하지 않는다.
				}
				break;
			case INQ_ANSWER:
				{
					std::unique_lock<std::mutex> lk(m_req);
					if(answer_map.find(id) != answer_map.end())
						answer_map[id].set_value(msg);
					lk.unlock();
				}
				break;
		}
	}
	catch(const std::exception& e)
	{
		Logger::log(e.what(), Logger::LogType::error);
	}
}

Expected<ByteQueue, StackErrorCode> MyConnector::Request(ByteQueue data)
{
	if (data.Size() <= 0)
		return StackErrorCode(ERR_OUT_OF_CAPACITY, __STACKINFO__);

	if(!isReadable())
		return StackErrorCode(ERR_CONNECTION_CLOSED, __STACKINFO__);

	ByteQueue answer;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dist(1, std::numeric_limits<unsigned int>::max());
	std::promise<ByteQueue> result_promise;
	std::future<ByteQueue> result_future = result_promise.get_future();

	std::unique_lock<std::mutex> lk(m_req);
	unsigned int id = dist(gen);	//Random Value
	while(answer_map.find(id) != answer_map.end())
		id = dist(gen);	//Random Value
	
	answer_map.insert({id, std::move(result_promise)});
	lk.unlock();

	data.push_head<unsigned int>(id);
	data.push_head<byte>(INQ_REQUEST);
	auto sec = StackErrorCode(Send(data), __STACKINFO__);
	if(!sec)
	{
		lk.lock();
		answer_map.erase(id);
		lk.unlock();
		return sec;
	}

	auto fs = result_future.wait_for(std::chrono::milliseconds(TIME_WAIT_ANSWER));
	lk.lock();	//성공하든 실패하든 promise는 삭제한다.
	answer_map.erase(id);
	lk.unlock();

	if(fs != std::future_status::ready)
		return StackErrorCode(ERR_TIMEOUT, __STACKINFO__);

	return result_future.get();
}

bool MyConnector::isAuthorized() const
{
	return m_authorized;
}