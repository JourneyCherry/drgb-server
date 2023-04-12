#include "KeyExchanger.hpp"

namespace mylib{
namespace security{

KeyExchanger::KeyExchanger() : host_key(nullptr)
{
	host_key = EVP_EC_gen("P-256");
	if(host_key == nullptr)
		throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
}

KeyExchanger::~KeyExchanger()
{
	EVP_PKEY_free(host_key);
}

Expected<std::vector<byte>, StackErrorCode> KeyExchanger::GetPublicKey()
{
	std::vector<byte> result;
	unsigned char *pubkey = nullptr;
	size_t pubkey_len = EVP_PKEY_get1_encoded_public_key(host_key, &pubkey);
	if(pubkey_len == 0)
		return StackErrorCode(ERR_get_error(), __STACKINFO__);

	result.assign(pubkey, pubkey + pubkey_len);
	OPENSSL_free(pubkey);

	return result;
}

Expected<std::vector<byte>, StackErrorCode> KeyExchanger::ComputeKey(std::vector<byte> peer_pub_key_byte)
{
	StackErrorCode sec;
	std::vector<byte> result;
	EVP_PKEY *peer_pub_key = nullptr;
	EVP_PKEY_CTX *ctx = nullptr;

	try
	{
		peer_pub_key = EVP_PKEY_new();
		if(peer_pub_key == nullptr)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		if(!EVP_PKEY_copy_parameters(peer_pub_key, host_key))
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		if(EVP_PKEY_set1_encoded_public_key(peer_pub_key, peer_pub_key_byte.data(), peer_pub_key_byte.size()) <= 0)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, host_key, nullptr);
		if(ctx == nullptr)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		if(EVP_PKEY_derive_init(ctx) <= 0)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		if(EVP_PKEY_derive_set_peer(ctx, peer_pub_key) <= 0)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);

		size_t secret_len = 0;
		if(EVP_PKEY_derive(ctx, nullptr, &secret_len) <= 0)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);
		result.assign(secret_len, 0);

		if(EVP_PKEY_derive(ctx, result.data(), &secret_len) <= 0)
			throw StackErrorCode(ERR_get_error(), __STACKINFO__);
	}
	catch(StackErrorCode ec)
	{
		sec = ec;
	}
	
	EVP_PKEY_free(peer_pub_key);
	EVP_PKEY_CTX_free(ctx);

	if(!sec)
		return sec;
	return result;
}

std::vector<byte> KeyExchanger::KDF(std::vector<byte> secret, size_t KeySize, std::vector<byte> sharedInfo)
{
	std::vector<byte> result(KeySize, 0);
	std::vector<OSSL_PARAM> params;
	std::exception_ptr eptr = nullptr;

	EVP_KDF *kdf = nullptr;
	EVP_KDF_CTX *ctx = nullptr;

	try
	{
		kdf = EVP_KDF_fetch(nullptr, "X963KDF", nullptr);
		if(kdf == nullptr)
			throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
		ctx = EVP_KDF_CTX_new(kdf);
		if(ctx == nullptr)
			throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);

        params.push_back(OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, (char*)SN_sha256, strlen(SN_sha256)));
        params.push_back(OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SECRET, secret.data(), secret.size()));
        params.push_back(OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, sharedInfo.data(), sharedInfo.size()));	//Optional
        params.push_back(OSSL_PARAM_construct_end());

		if(EVP_KDF_derive(ctx, result.data(), result.size(), params.data()) <= 0)
			throw ErrorCodeExcept(ErrorCode(ERR_get_error()), __STACKINFO__);
	}
	catch(...)
	{
		eptr = std::current_exception();
	}
	
	EVP_KDF_free(kdf);
	EVP_KDF_CTX_free(ctx);

	if(eptr)
		std::rethrow_exception(eptr);
	return result;
}

}
}