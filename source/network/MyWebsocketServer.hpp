#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "MyTCPServer.hpp"
#include "MyWebsocketClient.hpp"

using mylib::utils::ErrorCodeExcept;
using mylib::utils::StackTraceExcept;

/**
 * @brief Websocket for Server
 * 
 */
class MyWebsocketServer : public MyTCPServer
{
	protected:
		std::shared_ptr<MyClientSocket> GetClient(boost::asio::ip::tcp::socket&, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)>) override;

	public:
		/**
		 * @brief Constructor of Websocket Server Socket
		 * 
		 * @param p port number of accept socket. If this value is 0, random open port number will be selected and you can get it by calling 'GetPort()' method.
		 * @param t thread number for io_context. It should be at least 1.
		 */
		MyWebsocketServer(int p, int t);
		MyWebsocketServer(const MyWebsocketServer&) = delete;
		MyWebsocketServer(MyWebsocketServer&&) = delete;

		MyWebsocketServer& operator=(const MyWebsocketServer&) = delete;
		MyWebsocketServer& operator=(MyWebsocketServer&&) = delete;
};