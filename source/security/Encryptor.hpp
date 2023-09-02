#pragma once
#include <array>
#include <vector>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/ossl_typ.h>
#include "MyTypes.hpp"
#include "StackTraceExcept.hpp"

namespace mylib{
namespace security{

using utils::ErrorCode;
using utils::ErrorCodeExcept;

/**
 * @brief En/Decryption class using AES-256-CBC algorithm.
 * 
 */
class Encryptor
{
	public:
		static constexpr size_t KEY_SIZE = 32;
		static constexpr size_t IV_SIZE = 16;

	private:
		using Key_t = std::array<byte, KEY_SIZE>;
		using IV_t = std::array<byte, IV_SIZE>;

		bool Encryption;
		EVP_CIPHER_CTX *ctx;
		EVP_CIPHER *cipher;
		Key_t key;
		IV_t iv;

	public:
		Encryptor(bool);
		Encryptor(Key_t, IV_t, bool);
		~Encryptor();
		void SetKey(Key_t, IV_t);
		std::vector<byte> Crypt(std::vector<byte>);
		static std::pair<Key_t, IV_t> SplitKey(std::vector<byte>);
};

}
}