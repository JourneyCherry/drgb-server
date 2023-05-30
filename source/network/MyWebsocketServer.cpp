#include "MyWebsocketServer.hpp"

MyWebsocketServer::MyWebsocketServer(int p, int t) : MyTCPServer(p, t)
{
}

std::shared_ptr<MyClientSocket> MyWebsocketServer::GetClient(boost::asio::ip::tcp::socket& socket, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle)
{
	auto client = std::make_shared<MyWebsocketClient>(std::move(socket));
	client->Prepare(handle);

	return client;
}