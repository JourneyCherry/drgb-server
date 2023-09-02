#pragma once
#include <array>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include "StackTraceExcept.hpp"
#include "MyTypes.hpp"
#include "Expected.hpp"

namespace mylib{
namespace security{

using utils::ErrorCode;
using utils::StackErrorCode;
using utils::ErrorCodeExcept;
using utils::Expected;

/**
 * @brief Key Exchanger using ECDH(Elliptic Curve Diffie-Hellman) algorithm.
 * EC type : P-256
 * KDF : X963
 * 
 */
class KeyExchanger
{
	private:
		EVP_PKEY* host_key;

	public:
		KeyExchanger();
		~KeyExchanger();
		Expected<std::vector<byte>, StackErrorCode> GetPublicKey();
		Expected<std::vector<byte>, StackErrorCode> ComputeKey(std::vector<byte>);
		static std::vector<byte> KDF(std::vector<byte>, size_t, std::vector<byte> = std::vector<byte>());
};

}
}