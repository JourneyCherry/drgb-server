#include "MyTCPTLSClient.hpp"

MyTCPTLSClient::MyTCPTLSClient(int fd, SSL* sl, std::string addr) : MyClientSocket()
{
	socket_fd = fd;
	Address = addr;
	ssl = sl;

	if(!ssl)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	if(!SSL_set_fd(ssl, socket_fd))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	int ret_accept = 0;
	if((ret_accept = SSL_accept(ssl)) <= 0)
		throw ErrorCodeExcept(GetSSLErrorFromRet(ret_accept), __STACKINFO__);
}

MyTCPTLSClient::~MyTCPTLSClient()
{
	Close();
}

Expected<std::vector<byte>, ErrorCode> MyTCPTLSClient::RecvRaw()
{
	unsigned char buf[BUFSIZE];
	std::vector<byte> result_bytes;

	if(socket_fd < 0)
		return {ErrorCode{ERR_CONNECTION_CLOSED}};
	//int recvlen = read(socket_fd, buf, BUFSIZE);
	int recvlen = SSL_read(ssl, buf, BUFSIZE);
	if(recvlen == 0)
		return {ErrorCode{ERR_CONNECTION_CLOSED}};
	if(recvlen < 0)
	//	return {ErrorCode{errno}};
		return {GetSSLErrorFromRet(recvlen)};

	result_bytes.assign(buf, buf + recvlen);

	return result_bytes;
}

ErrorCode MyTCPTLSClient::SendRaw(const byte* bytes, const size_t &len)
{
	if(socket_fd < 0)
		return {ERR_CONNECTION_CLOSED};
	//int sendlen = send(socket_fd, bytes, len, 0);
	int sendlen = SSL_write(ssl, bytes, len);
	if(sendlen == 0)
		return {ERR_CONNECTION_CLOSED};
	if(sendlen < 0)
	//	return {errno};
		return {GetSSLErrorFromRet(sendlen)};

	return {SUCCESS};
}

StackErrorCode MyTCPTLSClient::Connect(std::string addr, int port)
{
	Close();	//이미 열려있는 소켓은 닫기.

	SSL_CTX *ctx = nullptr;

	try	//TODO : certificate, key를 파일이 아닌 직접 생성하는 방법도 찾아보자. 파일을 사용하면 Connector, Connectee가 같은 인증서/키를 사용하게됨.
	{
		ctx = SSL_CTX_new(TLS_client_method());
		if(!ctx)
			throw StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);

		SSL_CTX_set_ecdh_auto(ctx, 1);	//적절한 Elliptic Curve Diffie-Hellman 그룹 자동으로 고르기.
		/*
		//클라이언트에 인증서, 비밀키 불러오기.
		if(SSL_CTX_use_certificate_file(ctx, ConfigParser::GetString("SSL_CERT").c_str(), SSL_FILETYPE_PEM) <= 0)
			throw StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);
		if(SSL_CTX_use_PrivateKey_file(ctx, ConfigParser::GetString("SSL_KEY").c_str(), SSL_FILETYPE_PEM) <= 0)
			throw StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);
		if(!SSL_CTX_check_private_key(ctx))
			throw StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);
		*/
	}
	catch(StackErrorCode sec)
	{
		SSL_CTX_free(ctx);
		return sec;
	}

	ssl = SSL_new(ctx);
	SSL_CTX_free(ctx);
	if(!ssl)
		return StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0)
		return {errno, __STACKINFO__};

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, addr.c_str(), &(server_addr.sin_addr.s_addr)) <= 0)
	{
		struct addrinfo hints;
		struct addrinfo *serverinfo;

		char bport[6];

		memset(bport, 0, sizeof(port));
		sprintf(bport, "%hu", port);

		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		int result_getaddr = getaddrinfo(addr.c_str(), bport, &hints, &serverinfo);
		if (result_getaddr != 0)
			return {result_getaddr, __STACKINFO__};
		memcpy(&server_addr, serverinfo->ai_addr, sizeof(struct sockaddr));

		freeaddrinfo(serverinfo);
	}

	if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		return {errno, __STACKINFO__};

	if(!SSL_set_fd(ssl, socket_fd))
		return StackErrorCode(ErrorCode(ERR_get_error()), __STACKINFO__);

	int ret_connect = 0;
	if((ret_connect = SSL_connect(ssl)) < 0)
		return StackErrorCode(GetSSLErrorFromRet(ret_connect), __STACKINFO__);

	//if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	//	return {errno, __STACKINFO__};

	Address = std::string(inet_ntoa(server_addr.sin_addr)) + ":" + std::to_string(server_addr.sin_port);
	
	return {};
}

void MyTCPTLSClient::Close()
{
	if(ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
		ssl = nullptr;
	}
	if(socket_fd >= 0)
	{
		shutdown(socket_fd, SHUT_RDWR);
		socket_fd = -1;
	}
	recvbuffer.Clear();
}

ErrorCode MyTCPTLSClient::GetSSLErrorFromRet(int ret)
{
	switch(SSL_get_error(ssl, ret))	//For SSL_connect(), SSL_accept(), SSL_do_handshake(), SSL_read_ex(), SSL_read(), SSL_peek_ex(), SSL_peek(), SSL_shutdown(), SSL_write_ex(), SSL_write()
	{
		case SSL_ERROR_NONE:
			return {};
		case SSL_ERROR_SYSCALL:
			return {errno};
		case SSL_ERROR_ZERO_RETURN:
			return {ERR_CONNECTION_CLOSED};
		case SSL_ERROR_SSL:
		default:
			return ErrorCode(ERR_get_error());
	}
}