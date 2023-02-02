#include "MyWebsocketServer.hpp"

MyWebsocketServer::MyWebsocketServer(int p) : MyServerSocket(p)
{
	ioc = std::make_unique<boost::beast::net::io_context>(1);
	acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port));
	acceptor->set_option(boost::asio::socket_base::reuse_address(true));
}

MyWebsocketServer::~MyWebsocketServer()
{
	Close();
}

MyExpected<std::shared_ptr<MyClientSocket>, int> MyWebsocketServer::Accept()
{
	try
	{
		boost::asio::ip::tcp::socket socket{*ioc};
		acceptor->accept(socket);

		return {std::make_shared<MyWebsocketClient>(std::move(socket)), true};
	}
	catch(const boost::system::system_error &e)
	{
		int error_code = e.code().value();
		switch(error_code)
		{
			case EINVAL:	//shutdown(acceptor->native_handle(), SHUT_RDWR)에 의해 발생
				return {-1};
		}
		return {error_code};
	}
}

void MyWebsocketServer::Close()
{
	if(acceptor != nullptr)
	{
		shutdown(acceptor->native_handle(), SHUT_RDWR);
		acceptor->close();
		acceptor.reset();
	}
	if(ioc != nullptr)
		ioc.reset();
}