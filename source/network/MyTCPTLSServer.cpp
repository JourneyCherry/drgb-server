#include "MyTCPTLSServer.hpp"

MyTCPTLSServer::MyTCPTLSServer(int p) : MyServerSocket(p)
{
	int opt_reuseaddr = 1;

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
		throw ErrorCodeExcept(errno, __STACKINFO__);
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const int*)&opt_reuseaddr, sizeof(opt_reuseaddr)) < 0)
		throw ErrorCodeExcept(errno, __STACKINFO__);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		throw ErrorCodeExcept(errno, __STACKINFO__);
	if(listen(server_fd, LISTEN_SIZE) < 0)
		throw ErrorCodeExcept(errno, __STACKINFO__);

	ctx = SSL_CTX_new(TLS_server_method());
	if(!ctx)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	SSL_CTX_set_ecdh_auto(ctx, 1);	//적절한 Elliptic Curve Diffie-Hellman 그룹 자동으로 고르기.
	if(SSL_CTX_use_certificate_file(ctx, ConfigParser::GetString("SSL_CERT").c_str(), SSL_FILETYPE_PEM) <= 0)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	if(SSL_CTX_use_PrivateKey_file(ctx, ConfigParser::GetString("SSL_KEY").c_str(), SSL_FILETYPE_PEM) <= 0)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	if(!SSL_CTX_check_private_key(ctx))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);

	/*
	//클라이언트에게 인증서 요구하기.
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
	*/
}

MyTCPTLSServer::~MyTCPTLSServer()
{
	Close();
	SSL_CTX_free(ctx);
	EVP_cleanup();
	ERR_free_strings();
}

void MyTCPTLSServer::Close()
{
	if(server_fd >= 0)
	{
		shutdown(server_fd, SHUT_RDWR);
		server_fd = -1;
	}
}

Expected<std::shared_ptr<MyClientSocket>, ErrorCode> MyTCPTLSServer::Accept()
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof((struct sockaddr *)&client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	if(client_fd < 0)
	{
		ErrorCode ec(errno);
		if(!MyClientSocket::isNormalClose(ec))
			return ec;
	}

	return {std::make_shared<MyTCPTLSClient>(client_fd, SSL_new(ctx), std::string(inet_ntoa(client_addr.sin_addr)) + std::to_string(client_addr.sin_port))};
}