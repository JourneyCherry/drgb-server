#include "MyWebsocketServer.hpp"

MyWebsocketServer::MyWebsocketServer(int p) : ioc(), acceptor{ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port)}, MyServerSocket(p)
{
	acceptor.set_option(boost::asio::socket_base::reuse_address(true));
}

MyWebsocketServer::~MyWebsocketServer()
{
	Close();
}

MyExpected<std::shared_ptr<MyClientSocket>, int> MyWebsocketServer::Accept()
{
	try
	{
		boost::asio::ip::tcp::socket socket{ioc};
		acceptor.accept(socket);

		return {std::make_shared<MyWebsocketClient>(std::move(socket)), true};
	}
	catch(const boost::system::system_error &e)
	{
		int error_code = e.code().value();
		auto ec = e.code();
		if(ec == boost::asio::error::operation_aborted)
			return {-1};
		return {error_code};
	}
}

void MyWebsocketServer::Close()
{
	acceptor.close();
	ioc.stop();
}