#pragma once
#include <array>
#include "ConfigParser.hpp"
#include "MyServer.hpp"
#include "MyPostgres.hpp"
#include "Encoder.hpp"
#include "MyConnectorPool.hpp"
#include "MyCodes.hpp"
#include "Hasher.hpp"

using mylib::utils::Encoder;
using mylib::security::Hasher;
using mylib::utils::ErrorCodeExcept;

class MyAuth : public MyServer
{
	private:
		using dbsystem = MyPostgres;

		static constexpr Seed_t MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		static constexpr int MAX_MATCH_COUNT = 1;

		MyConnectorPool connector;
		std::string keyword_match;

	public:
		MyAuth();
		~MyAuth();
		void Open() override;
		void Close() override;
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void EnterProcess(std::shared_ptr<MyClientSocket>, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		ByteQueue AuthInquiry(ByteQueue);
};