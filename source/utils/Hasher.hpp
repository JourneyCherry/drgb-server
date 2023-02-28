#pragma once
#include "ByteQueue.hpp"
#include <openssl/sha.h>

namespace mylib{
namespace utils{

class Hasher
{
	public:
		static Hash_t sha256(ByteQueue);
		static Hash_t sha256(const unsigned char*, size_t);
};

}
}