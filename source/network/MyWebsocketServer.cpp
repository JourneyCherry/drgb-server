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
		std::shared_ptr<MyWebsocketClient> client = nullptr;
		boost::system::error_code result = boost::asio::error::operation_aborted;
		std::exception_ptr eptr = nullptr;

		acceptor.async_accept(socket, [&](boost::system::error_code ec){
			result = ec;
			if(ec)
				return;
			try
			{
				client = std::make_shared<MyWebsocketClient>(std::move(socket));
			}
			catch(...)
			{
				eptr = std::current_exception();
			}
		});

		if(ioc.stopped())
			ioc.restart();
		ioc.run();

		if(result)
		{
			if(result == boost::asio::error::operation_aborted)
				return {-1};
			return {result.value()};
		}
		if(eptr)
			throw eptr;
		if(!client)		//MyWebsocketServer::Close()의 호출로 종료되면 보통 위의 result에서 걸려 나가지만, 혹시모를 예외사항을 위해 
			return {-1};//한번 더 검증을 수행함.
		return {client, true};
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
	ioc.stop();
}