#pragma once
#include <hiredis/hiredis.h>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

class MyRedisReply
{
	public:
		int type;
		long long integer;
		double dval;
		std::string str;
		std::array<char, 4> vtype;
		std::vector<MyRedisReply> elements;
		
		MyRedisReply(redisReply*);
		MyRedisReply(const MyRedisReply&) = default;
		MyRedisReply(MyRedisReply&&) noexcept = default;
		~MyRedisReply() = default;

		MyRedisReply& operator=(const MyRedisReply&) = default;
		MyRedisReply& operator=(MyRedisReply&&) noexcept = default;
};