#include "Hasher.hpp"

namespace mylib{
namespace security{

Hash_t Hasher::sha256(ByteQueue bytes)
{
	return sha256(bytes.data(), bytes.Size());
}

Hash_t Hasher::sha256(const unsigned char *data, size_t size)
{
	Hash_t digest;
	SHA256(data, size, digest.data());	//리턴값은 result의 주소값이다.

	return digest;
}

}
}