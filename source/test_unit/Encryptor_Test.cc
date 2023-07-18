#include <gtest/gtest.h>
#include "Encryptor.hpp"
#include "KeyExchanger.hpp"

using mylib::security::Encryptor;
using mylib::security::KeyExchanger;
using mylib::utils::StackErrorCode;

TEST(EncryptorTest, KeyExchangeTest)
{
	KeyExchanger a;
	KeyExchanger b;

	auto apkey = a.GetPublicKey();
	auto bpkey = b.GetPublicKey();

	ASSERT_TRUE(apkey.isSuccessed()) << apkey.error().message_code();
	ASSERT_TRUE(bpkey.isSuccessed()) << bpkey.error().message_code();

	auto aresult = a.ComputeKey(*bpkey);
	auto bresult = b.ComputeKey(*apkey);

	ASSERT_TRUE(aresult.isSuccessed()) << aresult.error().message_code();
	ASSERT_TRUE(bresult.isSuccessed()) << bresult.error().message_code();

	auto aderived = KeyExchanger::KDF(*aresult, 48);
	auto bderived = KeyExchanger::KDF(*bresult, 48);

	ASSERT_EQ(aderived, bderived);
}

class EncryptorTestFixture : public ::testing::Test
{
	protected:
		void SetUp() override
		{
			KeyExchanger ake;
			KeyExchanger bke;

			auto apkey = ake.GetPublicKey();
			auto bpkey = bke.GetPublicKey();

			auto aresult = ake.ComputeKey(*bpkey);
			auto bresult = bke.ComputeKey(*apkey);

			auto aderived = KeyExchanger::KDF(*aresult, 48);
			auto bderived = KeyExchanger::KDF(*bresult, 48);

			auto [akey, aiv] = Encryptor::SplitKey(aderived);
			auto [bkey, biv] = Encryptor::SplitKey(bderived);

			en.SetKey(akey, aiv);
			de.SetKey(bkey, biv);
		}

	public:
		EncryptorTestFixture() : en(true), de(false){}
		Encryptor en;
		Encryptor de;

		std::vector<byte> StrToVec(std::string str)
		{
			std::vector<byte> result;
			for(auto ch : str)
				result.push_back(ch);
			return result;
		}

		std::string VecToStr(std::vector<byte> vec)
		{
			std::string result;
			for(auto b : vec)
				result.push_back(b);
			return result;
		}
};

TEST_F(EncryptorTestFixture, CryptionTest)
{
	std::vector<std::string> strings = {
		"Hello World!",
		"Hello Another World",
		"This is not the Test"
	};

	for(auto plain : strings)
	{
		auto plainvec = StrToVec(plain);
		auto cipher = en.Crypt(plainvec);
		ASSERT_NE(plainvec, cipher);	//Plain Text and Cipher should not the same.

		auto receive = de.Crypt(cipher);
		ASSERT_STREQ(plain.c_str(), VecToStr(receive).c_str());
	}
}