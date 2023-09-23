#pragma once
#include <array>
#include <regex>
#include "ConfigParser.hpp"
#include "MyServer.hpp"
#include "Encoder.hpp"
#include "MyCodes.hpp"
#include "Hasher.hpp"

using mylib::utils::Encoder;
using mylib::security::Hasher;
using mylib::utils::ErrorCodeExcept;

/**
 * @brief Authentication Server.
 * **Spec**
 *  - Socket Type : Websocket
 *  - Process
 *   - Receive ID and PWD. If it is in DB, transfer client to 'Match Server' using 'Redis'
 * 
 */
class MyAuth : public MyServer
{
	private:
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