#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "MyTypes.hpp"

namespace mylib{
namespace utils{

/**
 * @brief Collection of Encoding/Decoding Functions. It doesn't use padding character for data-saving.
 * 
 */
class Encoder
{
	private:
		static const char* charset;
		static const byte mapset[256];
	public:
		/**
		 * @brief Encode byte data.
		 * 
		 * @param bytes data memory pointer.
		 * @param len data memory size.
		 * @return std::string Encoded characters in std::string type.
		 */
		static std::string EncodeBase64(const byte *bytes, size_t len);
		/**
		 * @brief Encode byte data.
		 * 
		 * @param bytes data in Vector STL Container.
		 * @return std::string Encoded characters in std::string type.
		 */
		static std::string EncodeBase64(std::vector<byte> bytes);
		/**
		 * @brief Decode string data.
		 * 
		 * @param str Encoded characters in std::string type.
		 * @return std::vector<byte> decoded byte data in Vector STL Container.
		 */
		static std::vector<byte> DecodeBase64(std::string str);
	private:
		//Base64//
		static byte cut_6_2(byte);
		static byte cut_2_6(byte);
		static byte mix_2_4(byte, byte);
		static byte mix_4_2(byte, byte);

		static byte mix_6_2(byte, byte);
		static byte mix_4_4(byte, byte);
		static byte mix_2_6(byte, byte);
		////
};

}
}