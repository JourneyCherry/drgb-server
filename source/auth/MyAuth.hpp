#pragma once
#include <array>
#include "MyConfigParser.hpp"
#include "MyServer.hpp"
#include "MyPostgres.hpp"
#include "MyBase64.hpp"
#include "MyConnector.hpp"
#include "MyCodes.hpp"
#include "MyCrypto.hpp"

class MyAuth : public MyServer
{
	private:
		using Pwd_Hash_t = std::array<byte, 32>;
		using dbsystem = MyPostgres;
		using Encoder = MyCommon::Base64;

		static constexpr Seed_t MACHINE_ID = 1;	//TODO : 이 부분은 추후 다중서버로 확장 시 변경 필요.
		static constexpr int MAX_MATCH_COUNT = 1;

		MyConnector connector_match;	//TODO : match 서버가 여럿일 경우 처리 필요.

	public:
		MyAuth();
		~MyAuth();
		void Open() override;
		void Close() override;
		void ClientProcess(std::shared_ptr<MyClientSocket>) override;
};