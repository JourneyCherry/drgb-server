#pragma once
#include <array>
#include "ConfigParser.hpp"
#include "MyServer.hpp"
#include "MyPostgres.hpp"
#include "Encoder.hpp"
#include "MyConnector.hpp"
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

		MyConnector connector_match;	//TODO : match 서버가 여럿일 경우 처리 필요.

	public:
		MyAuth();
		~MyAuth();
		void Open() override;
		void Close() override;
		void ClientProcess(std::shared_ptr<MyClientSocket>) override;
};