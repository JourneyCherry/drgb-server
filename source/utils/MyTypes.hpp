#pragma once
#include <array>
#include <cstdint>
#include <openssl/sha.h>

typedef unsigned char byte;
typedef int32_t Int;
typedef uint32_t UInt;
typedef unsigned long long Account_ID_t;
typedef int Seed_t;

typedef std::array<byte, SHA256_DIGEST_LENGTH> Hash_t;	//sha256 : 256bit == 32byte