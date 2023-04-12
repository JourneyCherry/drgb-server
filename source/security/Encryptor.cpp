#include "Encryptor.hpp"

namespace mylib{
namespace security{

Encryptor::Encryptor(bool isEncrypt) : ctx(nullptr), cipher(nullptr), Encryption(isEncrypt)
{
	ctx = EVP_CIPHER_CTX_new();
	if(ctx == nullptr)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	cipher = EVP_CIPHER_fetch(nullptr, "AES-256-CBC", nullptr);
	if(cipher == nullptr)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
}

Encryptor::Encryptor(std::array<byte, KEY_SIZE> k, std::array<byte, IV_SIZE> i, bool isEncrypt) : Encryptor(isEncrypt)
{
	SetKey(k, i);
}

Encryptor::~Encryptor()
{
	EVP_CIPHER_CTX_free(ctx);
	EVP_CIPHER_free(cipher);
}

void Encryptor::SetKey(std::array<byte, KEY_SIZE> k, std::array<byte, IV_SIZE> i)
{
	key = k;
	iv = i;

	if(!EVP_CipherInit_ex2(ctx, cipher, key.data(), iv.data(), Encryption, nullptr))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
		
	if(!EVP_CIPHER_CTX_set_padding(ctx, 1))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
}

std::vector<byte> Encryptor::Crypt(std::vector<byte> data)
{
	std::vector<byte> result;
	int outlen = 0;
	int padlen = 0;
	int maxlength = (std::floor((double)(data.size()) / (double)IV_SIZE) + 1) * IV_SIZE;

	result.resize(maxlength);

	if(!EVP_CipherUpdate(ctx, result.data(), &outlen, data.data(), data.size()))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);

	if(!EVP_CipherFinal_ex(ctx, result.data() + outlen, &padlen))
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);

	/*
	if(Encryption)
		std::copy_n(result.end() - IV_SIZE, IV_SIZE, iv.begin());
	else
		std::copy_n(data.end() - IV_SIZE, IV_SIZE, iv.begin());
	*/

	result.resize(outlen + padlen);

	return result;
}

std::pair<Encryptor::Key_t, Encryptor::IV_t> Encryptor::SplitKey(std::vector<byte> secret)
{
	if(secret.size() != KEY_SIZE + IV_SIZE)
		return {};
	
	Key_t key;
	IV_t iv;
	std::copy_n(secret.begin(), KEY_SIZE, key.begin());
	std::copy_n(secret.begin() + KEY_SIZE, IV_SIZE, iv.begin());

	return {key, iv};
}

}
}