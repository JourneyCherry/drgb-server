#include <gtest/gtest.h>
#include <cmath>
#include <queue>
#include "ByteQueue.hpp"

using mylib::utils::ByteQueue;

TEST(ByteQueueTest, TypeTest)
{
	int8_t i8 = 1 * 8;				//byte
	int16_t i16 = 2 * 16;			//short ~ int
	int32_t i32 = 3 * 32;			//int ~ long int
	int64_t i64 = 4 * 64;			//long int ~ long long int
	_Float32 f32 = 5 * M_PI;		//float
	_Float64 f64 = 6 * M_PI;		//double
	_Float64x f64x = 7 * M_PI;		//long double
	__float80 f80 = 8 * M_PI;		//80 bit float
	__float128 f128 = 9 * M_PI;		//128 bit float
	std::string str = "hello world";//string

	ByteQueue bq;
	bq.push<int8_t>(i8);
	bq.push<int16_t>(i16);
	bq.push<int32_t>(i32);
	bq.push<int64_t>(i64);
	bq.push<_Float32>(f32);
	bq.push<_Float64>(f64);
	bq.push<_Float64x>(f64x);
	bq.push<__float80>(f80);
	bq.push<__float128>(f128);
	bq.push<std::string>(str);

	EXPECT_EQ(bq.Size(), 
	sizeof(int8_t) + 
	sizeof(int16_t) + 
	sizeof(int32_t) + 
	sizeof(int64_t) + 
	sizeof(_Float32) + 
	sizeof(_Float64) + 
	sizeof(_Float64x) + 
	sizeof(__float80) + 
	sizeof(__float128) + 
	str.length()
	);

	ASSERT_EQ(bq.pop<int8_t>(), i8);
	ASSERT_EQ(bq.pop<int16_t>(), i16);
	ASSERT_EQ(bq.pop<int32_t>(), i32);
	ASSERT_EQ(bq.pop<int64_t>(), i64);
	ASSERT_FLOAT_EQ(bq.pop<_Float32>(), f32);
	ASSERT_DOUBLE_EQ(bq.pop<_Float64>(), f64);
	ASSERT_DOUBLE_EQ(bq.pop<_Float64x>(), f64x);
	ASSERT_FLOAT_EQ(bq.pop<__float80>(), f80);
	ASSERT_FLOAT_EQ(bq.pop<__float128>(), f128);
	ASSERT_STREQ(bq.popstr().c_str(), str.c_str());
}

TEST(ByteQueueTest, EndianTest)
{
	uint64_t little = 0x0123456789abcdef;
	uint64_t big = 0xefcdab8967452301;

	ByteQueue bq;
	bq.push<uint64_t>(little, true);

	ASSERT_EQ(bq.pop<uint64_t>(false, false), big);
}

TEST(ByteQueueTest, SplitTest)
{
	ByteQueue bq;
	uint64_t source = 0x0123456789abcdef;
	const size_t size = sizeof(uint64_t);	//8
	bq.push<uint64_t>(source, false);	//Big Endian

	ASSERT_EQ(bq.Size(), size);
	ASSERT_EQ(bq.pop<uint64_t>(true, false), source);

	std::array<uint8_t, size> series_8{
		0x01,	//0001	//1
		0x23,	//0043	//35
		0x45,	//0105	//69
		0x67,	//0147	//103
		0x89,	//0211	//137
		0xab,	//0253	//171
		0xcd,	//0315	//205
		0xef	//0357	//239
	};

	std::array<uint16_t, size/2> series_16{
		0x0123,	//0000 443	//291
		0x4567,	//0042 547	//17767
		0x89ab,	//0104 653	//35243
		0xcdef	//0146 757	//52719
	};

	std::queue<ByteQueue> buffer;

	for(int i = 0;i<size/2;i++)
	{
		ByteQueue b16 = bq.split(i*2, i*2 + sizeof(uint16_t));
		ASSERT_EQ(b16.Size(), sizeof(uint16_t));
		ASSERT_EQ(b16.pop<uint16_t>(true, false), series_16[i]);
		buffer.push(b16);
	}

	for(int i = 0;i<size/2;i++)
	{
		ByteQueue top = buffer.front();
		buffer.pop();
		EXPECT_EQ(top.pop<uint8_t>(false, false), series_8[i*2]);
		EXPECT_EQ(top.split().pop<uint8_t>(false, false), series_8[i*2 + 1]);
	}
}