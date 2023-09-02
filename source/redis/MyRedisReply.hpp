#pragma once
#include <hiredis/hiredis.h>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

/**
 * @brief Cachable Redis Reply class to store 'redisReply*' in memory.
 * 
 */
class MyRedisReply
{
	public:
		/**
		 * @brief Type of the data it stores.
		 * 
		 */
		int type;
		/**
		 * @brief Integer value of the data. The 'type' should be 'REDIS_REPLY_INTEGER'(3)
		 * 
		 */
		long long integer;
		/**
		 * @brief Double value of the data. The 'type' should be 'REDIS_REPLY_DOUBLE'(7)
		 * 
		 */
		double dval;
		/**
		 * @brief String value of the data. The 'type' should be 'REDIS_REPLY_STRING'(1)
		 * 
		 */
		std::string str;
		/**
		 * @brief Verbatim String value of the data. The 'type' should be 'REDIS_REPLY_VERB'(14)
		 * 
		 */
		std::array<char, 4> vtype;
		/**
		 * @brief Multi bulk reply. The 'type' should be 'REDIS_REPLY_ARRAY'(2)
		 * 
		 */
		std::vector<MyRedisReply> elements;
		
		/**
		 * @brief Construct a new MyRedisReply object.
		 * 
		 * @param reply target pointer for storing before being free.
		 */
		MyRedisReply(redisReply *reply);
		MyRedisReply(const MyRedisReply&) = default;
		MyRedisReply(MyRedisReply&&) noexcept = default;
		~MyRedisReply() = default;

		MyRedisReply& operator=(const MyRedisReply&) = default;
		MyRedisReply& operator=(MyRedisReply&&) noexcept = default;
};