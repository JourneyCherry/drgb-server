#include "MyWebsocketServer.hpp"

MyWebsocketServer::MyWebsocketServer(int p) : ioc(), acceptor{ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)port)}, MyServerSocket(p)
{
	acceptor.set_option(boost::asio::socket_base::reuse_address(true));
}

MyWebsocketServer::~MyWebsocketServer()
{
	Close();
}

Expected<std::shared_ptr<MyClientSocket>> MyWebsocketServer::Accept()
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
		catch(StackTraceExcept e)
		{
			e.stack(__STACKINFO__);
			eptr = std::make_exception_ptr(e);
		}
		catch(const std::exception &e)
		{
			eptr = std::make_exception_ptr(StackTraceExcept(e.what(), __STACKINFO__));
		}
	});

	if(ioc.stopped())
		ioc.restart();
	ioc.run();

	if(result)
	{
		if(result == boost::asio::error::operation_aborted)
			return {};
		throw ErrorCodeExcept(result, __STACKINFO__);
	}
	if(eptr)
		throw eptr;
	if(!client)		//MyWebsocketServer::Close()의 호출로 종료되면 보통 위의 result에서 걸려 나가지만, 혹시모를 예외사항을 위해 
		return {};//한번 더 검증을 수행함.
	return {client, true};
}

void MyWebsocketServer::Close()
{
	ioc.stop();
}