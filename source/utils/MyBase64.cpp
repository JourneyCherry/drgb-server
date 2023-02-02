#include "MyBase64.hpp"

const char* MyCommon::Base64::charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const byte MyCommon::Base64::mapset[256] = 
{
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//00-0F
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//10255F
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,	//20-2F
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,	//30-3F
	255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,	//40-4F
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,	//50-5F
	255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,	//60-6F
	 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,	//70-7F
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//80-8F
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//90-9F
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//A0-AF
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//B0-BF
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//C0-CF
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//D0-DF
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,	//E0-EF
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255	//F0-FF
};

std::string MyCommon::Base64::Encode(const byte *bytes, size_t len)
{
	std::string result;

	for(int i = 0;i<len/3;i++)
	{
		byte c1 = bytes[3*i];
		byte c2 = bytes[3*i + 1];
		byte c3 = bytes[3*i + 2];

		result.push_back(charset[cut_6_2(c1)]);
		result.push_back(charset[mix_2_4(c1, c2)]);
		result.push_back(charset[mix_4_2(c2, c3)]);
		result.push_back(charset[cut_2_6(c3)]);
	}
	switch(len%3)
	{
		case 0:
			break;
		case 1:
			{
				byte c1 = bytes[len-1];

				result.push_back(charset[cut_6_2(c1)]);
				result.push_back(charset[mix_2_4(c1, 0)]);
			}
			break;
		case 2:
			{
				byte c1 = bytes[len - 2];
				byte c2 = bytes[len - 1];

				result.push_back(charset[cut_6_2(c1)]);
				result.push_back(charset[mix_2_4(c1, c2)]);
				result.push_back(charset[mix_4_2(c2, 0)]);
			}
			break;
	}

	return result;
}

std::string MyCommon::Base64::Encode(std::vector<byte> bytes)
{
	return Encode(bytes.data(), bytes.size());
}

std::vector<byte> MyCommon::Base64::Decode(std::string str)
{
	std::vector<byte> result;
	
	int phase = 0;
	byte prev;
	byte b;
	for(char c : str)
	{
		int d = mapset[(int)c];
		if(d >= 255)
			throw std::runtime_error("Unknown Character : " + c);
		byte uc = (byte)d;
		switch(phase)
		{
			case 0:
				break;
			case 1:
				result.push_back(mix_6_2(prev, uc));
				break;
			case 2:
				result.push_back(mix_4_4(prev, uc));
				break;
			case 3:
				result.push_back(mix_2_6(prev, uc));
				phase = -1;
				break;
		}
		phase++;
		prev = uc;
	}
	if(phase -1 == 0)
		throw std::runtime_error("Broken Data");

	return result;
}

byte MyCommon::Base64::cut_6_2(byte s)
{
	return (s & 0xFC) >> 2;
}
byte MyCommon::Base64::cut_2_6(byte s)
{
	return s & 0x3F;
}
byte MyCommon::Base64::mix_2_4(byte lhs, byte rhs)
{
	return (lhs & 0x03) << 4 | (rhs & 0xF0) >> 4;
}
byte MyCommon::Base64::mix_4_2(byte lhs, byte rhs)
{
	return (lhs & 0x0F) << 2 | (rhs & 0xC0) >> 6;
}
byte MyCommon::Base64::mix_6_2(byte lhs, byte rhs)
{
	return lhs << 2 | (rhs & 0x30) >> 4;
}
byte MyCommon::Base64::mix_4_4(byte lhs, byte rhs)
{
	return (lhs & 0x0F) << 4 | (rhs & 0x3C) >> 2;
}
byte MyCommon::Base64::mix_2_6(byte lhs, byte rhs)
{
	return (lhs & 0x03) << 6 | rhs;
}