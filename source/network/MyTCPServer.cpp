#include "MyTCPServer.hpp"

MyTCPServer::MyTCPServer(int p, int t) : acceptor{ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), (unsigned short)p)}, MyServerSocket(p, t)
{
	acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	port = acceptor.local_endpoint().port();
}

MyTCPServer::~MyTCPServer()
{
	Close();
}

bool MyTCPServer::is_open()
{
	return acceptor.is_open();
}

void MyTCPServer::CloseSocket()
{
	acceptor.close();
	safe_loop([&](std::shared_ptr<MyClientSocket> client){ client->Close(); });
}

void MyTCPServer::StartAccept(std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle)
{
	enterHandle = handle;
	acceptor.async_accept(std::bind(&MyTCPServer::Accept_Handle, this, std::placeholders::_1, std::placeholders::_2));
	StartThread();
}

void MyTCPServer::Accept_Handle(boost::system::error_code error_code, boost::asio::ip::tcp::socket socket)
{
	if(error_code.failed())
	{
		ErrorCode ec(error_code);
		if(MyClientSocket::isNormalClose(ec))
			return;
		throw ErrorCodeExcept(ec, __STACKINFO__);
	}

	std::exception_ptr eptr = nullptr;
	try
	{
		AddClient(GetClient(socket, enterHandle));
	}
	catch(...)
	{
		eptr = std::current_exception();
	}
	if(is_open())
		acceptor.async_accept(std::bind(&MyTCPServer::Accept_Handle, this, std::placeholders::_1, std::placeholders::_2));

	if(eptr)
		std::rethrow_exception(eptr);
}

std::shared_ptr<MyClientSocket> MyTCPServer::GetClient(boost::asio::ip::tcp::socket& socket, std::function<void(std::shared_ptr<MyClientSocket>, ErrorCode)> handle)
{
	auto client = std::make_shared<MyTCPClient>(std::move(socket));
	client->Prepare(handle);

	return client;
}