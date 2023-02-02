#pragma once
#include "MyBytes.hpp"
#include <openssl/sha.h>

namespace MyCommon
{
	class Hash
	{
		public:
			static Hash_t sha256(MyBytes);
			static Hash_t sha256(const unsigned char*, size_t);
	};
}