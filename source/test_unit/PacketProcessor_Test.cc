#include <gtest/gtest.h>
#include <vector>
#include "PacketProcessor.hpp"

using mylib::utils::PacketProcessor;

TEST(PacketProcessorTest, CapsulationTest)
{
	std::vector<byte> plain = {0x01, 0x02, 0x03, 0x04};
	std::vector<byte> cipher = {0x01, 0x02, 0x03, 0x04, 0xee, 0xff};
	EXPECT_EQ(plain, PacketProcessor::decapsulate(cipher));
	EXPECT_EQ(cipher, PacketProcessor::encapsulate(plain));

	plain = {0x01, 0xee, 0x03, 0xff, 0x05};
	cipher = {0x01, 0xee, 0xee, 0x03, 0xff, 0x05, 0xee, 0xff};
	EXPECT_EQ(plain, PacketProcessor::decapsulate(cipher));
	EXPECT_EQ(cipher, PacketProcessor::encapsulate(plain));
	
	plain = {0x01, 0xee, 0xff, 0x04};
	cipher = {0x01, 0xee, 0xee, 0xff, 0x04, 0xee, 0xff};
	EXPECT_EQ(plain, PacketProcessor::decapsulate(cipher));
	EXPECT_EQ(cipher, PacketProcessor::encapsulate(plain));
}

TEST(PacketProcessorTest, FragmentTest)
{
	std::vector<std::vector<byte>> plains = {
		{0x01, 0x02, 0x03, 0x04},
		{0xee, 0xff},
		{0x01, 0x02, 0xee, 0xff, 0x05, 0x06},
		{0xee, 0xee, 0xee, 0xee, 0xee, 0xff, 0xee, 0xff}
	};
	std::vector<byte> cipher;
	for(auto plain : plains)
	{
		auto msg = PacketProcessor::encapsulate(plain);
		cipher.insert(cipher.end(), msg.begin(), msg.end());
	}
	PacketProcessor buffer;

	auto iter = cipher.begin();
	auto count = 0;

	while(iter != cipher.end())
	{
		buffer.Recv(&(*iter), 1);
		iter++;
		if(buffer.isMsgIn())
		{
			EXPECT_EQ(plains[count], buffer.GetMsg());
			count++;
		}
	}
	EXPECT_EQ(plains.size(), count);
}

TEST(PacketProcessorTest, BigdataTest)
{
	constexpr int data_count = 65536;
	std::vector<byte> bigdata;
	for(int i = 0;i<data_count;i++)
		bigdata.push_back(i%256);

	std::vector<byte> cipher = PacketProcessor::encapsulate(bigdata);

	PacketProcessor buffer;

	buffer.Recv(cipher.data(), cipher.size());
	ASSERT_TRUE(buffer.isMsgIn());
	if(buffer.isMsgIn())
	{
		auto plain = buffer.GetMsg();
		ASSERT_EQ(plain, bigdata);
	}
}