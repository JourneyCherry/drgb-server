#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "MyTypes.hpp"

namespace mylib{
namespace utils{

class Encoder
{
	//Base64//
	private:
		static const char* charset;
		static const byte mapset[256];
	public:
		static std::string EncodeBase64(const byte*, size_t);
		static std::string EncodeBase64(std::vector<byte>);
		static std::vector<byte> DecodeBase64(std::string);
	private:
		static byte cut_6_2(byte);
		static byte cut_2_6(byte);
		static byte mix_2_4(byte, byte);
		static byte mix_4_2(byte, byte);

		static byte mix_6_2(byte, byte);
		static byte mix_4_4(byte, byte);
		static byte mix_2_6(byte, byte);
	//Base64//
};

}
}