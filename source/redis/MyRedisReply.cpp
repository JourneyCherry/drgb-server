#include "MyRedisReply.hpp"

MyRedisReply::MyRedisReply(redisReply *reply)
 : type(0),
	integer(0),
	dval(0),
	str(),
	vtype({0, 0, 0, 0}),
	elements()
{
	if(reply == nullptr)
		return;
	type = reply->type;
	integer = reply->integer;
	dval = reply->dval;
	str = std::string(reply->str, reply->len);
	std::copy_n(reply->vtype, 4, vtype.begin());
	for(int i = 0;i<reply->elements;i++)
		elements.emplace_back(reply->element[i]);
}