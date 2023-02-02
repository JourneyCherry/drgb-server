#include "MyCrypto.hpp"

Hash_t MyCommon::Hash::sha256(MyBytes bytes)
{
	return sha256(bytes.data(), bytes.Size());
}

Hash_t MyCommon::Hash::sha256(const unsigned char *data, size_t size)
{
	Hash_t digest;
	SHA256(data, size, digest.data());	//리턴값은 result의 주소값이다.

	return digest;
}