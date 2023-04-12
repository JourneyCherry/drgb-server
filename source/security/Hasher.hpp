#pragma once
#include <openssl/sha.h>
#include "ByteQueue.hpp"

namespace mylib{
namespace security{

using utils::ByteQueue;

class Hasher
{
	public:
		static Hash_t sha256(ByteQueue);
		static Hash_t sha256(const unsigned char*, size_t);
};

}
}