#include <gtest/gtest.h>
#include "Encoder.hpp"

using mylib::utils::Encoder;

TEST(EncoderTest, FragmentTest)
{
	std::string encoded = "RQ";
	std::vector<byte> decoded = {'E'};
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));

	encoded = "RTE";
	decoded.push_back('1');
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));

	encoded = "RTFM";
	decoded.push_back('L');
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));
	
	encoded = "RTFMUg";
	decoded.push_back('R');
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));
	
	encoded = "RTFMUmE";
	decoded.push_back('a');
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));
	
	encoded = "RTFMUmFF";
	decoded.push_back('E');
	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64(decoded).c_str());
	EXPECT_EQ(decoded, Encoder::DecodeBase64(encoded));
}

TEST(EncoderTest, BrokenTest)
{
	EXPECT_ANY_THROW(Encoder::DecodeBase64("-=\""));
	EXPECT_ANY_THROW(Encoder::DecodeBase64("a"));
}

TEST(EncoderTest, SentenceTest)
{
	std::string plain = "The Quick Brown Fox Jumps Over The Lazy Dog";
	std::string encoded = "VGhlIFF1aWNrIEJyb3duIEZveCBKdW1wcyBPdmVyIFRoZSBMYXp5IERvZw";

	EXPECT_STREQ(encoded.c_str(), Encoder::EncodeBase64((byte*)plain.c_str(), plain.size()).c_str());
	EXPECT_STREQ(plain.c_str(), (char*)Encoder::DecodeBase64(encoded).data());

}