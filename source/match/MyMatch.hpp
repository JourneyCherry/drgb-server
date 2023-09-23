#pragma once
#include "MyServer.hpp"
#include "MatchServiceServer.hpp"
#include "ConfigParser.hpp"
#include "MyCodes.hpp"
#include "MyMatchMaker.hpp"
#include "DeMap.hpp"

using mylib::utils::DeMap;
using mylib::utils::Expected;

/**
 * @brief Match Server.
 * **Spec**
 * 	- Socket Type : Websocket
 *  - Process
 *   - Receive Cookie and Check it with 'Redis'. If found, Send Player Information, or Reject Connection
 *   - Receive Match Participation or not.
 *   - Wait for Match Making. If found, Transfer match and player to Battle Server.(Least Connection Load-balancing)
 * 
 */
class MyMatch : public MyServer
{
	private:
		static const std::string self_keyword;	//keyword for 'MatchService' of gRPC
		std::chrono::milliseconds rematch_delay;	//Matching delay.(millisecond)
		DeMap<Account_ID_t, Hash_t, std::weak_ptr<MyClientSocket>> sessions;	//Local Server Session

		MatchServiceServer MatchService;
		std::thread t_matchmaker;
		MyMatchMaker matchmaker;
		std::mutex matchmaker_mtx;
		std::condition_variable matchmaker_cv;

	public:
		/**
		 * @brief Constructor of Match Server. Automatically register 'MatchService'.
		 * 
		 */
		MyMatch();
		~MyMatch();
		/**
		 * @brief Open and Start Match Server. automatically start 'MatchMake()'.
		 * 
		 */
		void Open() override;
		void Close() override;

	private:
		void AcceptProcess(std::shared_ptr<MyClientSocket>, ErrorCode) override;
		void AuthenticateProcess(std::shared_ptr<MyClientSocket>, ByteQueue, ErrorCode);
		void ClientProcess(std::shared_ptr<MyClientSocket>, Account_ID_t);
		/**
		 * @brief Session Process for HeartBeat and Redis.
		 * 
		 * @param target_client client socket
		 * @param account_id account's id
		 * @param cookie client's cookie value
		 * 
		 */
		void SessionProcess(std::shared_ptr<MyClientSocket> target_client, Account_ID_t account_id, Hash_t cookie);

		/**
		 * @brief Match Making method. It runs on 't_matchmaker' thread and sleeps until there are two players or more waiting. After awake, make match at 'rematch_delay' time intervals.
		 * 
		 */
		void MatchMake();

	protected:
		bool CheckAccount(Account_ID_t) override;
		std::map<std::string, size_t> GetConnectUsage() override;
};