#pragma once
#include <array>
#include "ConfigParser.hpp"
#include "MyServer.hpp"
#include "MyPostgres.hpp"
#include "MyRedis.hpp"
#include "Encoder.hpp"
#include "MyCodes.hpp"
#include "Hasher.hpp"

using mylib::utils::Encoder;
using mylib::security::Hasher;
using mylib::utils::ErrorCodeExcept;

class MyAuth : public MyServer
{
	private:
		using dbsystem = MyPostgres;

		MyRedis redis;
		const std::string keyword_match = "match";

	public:
		MyAuth();
		~MyAuth();
		void Open() override;
		void Close() override;
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void ClientProcess(std::shared_ptr<MyClientSocket>);
		void SessionProcess(std::shared_ptr<MyClientSocket>);
};