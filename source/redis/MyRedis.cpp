#include "MyRedis.hpp"

ErrorCode MyRedis::GetErrorCode()
{
	return GetErrorCode(ctx->err, ctx->errstr);
}

ErrorCode MyRedis::GetErrorCode(int code, std::string msg)
{
	ErrorCode ec(code);
	ec.SetMessage("(MyRedis)" + msg);

	return ec;
}

MyRedis::~MyRedis()
{
	if(ctx)
		Close();
}

ErrorCode MyRedis::Connect(std::string addr, int port)
{
	if(ctx)
		Close();

	redisOptions opt = {0};
	REDIS_OPTIONS_SET_TCP(&opt, addr.c_str(), port);
	opt.options |= REDIS_OPT_PREFER_IPV4 | REDIS_OPT_REUSEADDR;

	std::unique_lock<std::mutex> lk(mtx);
	ctx = redisConnectWithOptions(&opt);
	if(!ctx)
		return ErrorCode(ERR_CONNECTION_CLOSED);
	if(ctx->err)
		return GetErrorCode();
	
	return ErrorCode(SUCCESS);
}

void MyRedis::Close()
{
	if(!ctx)
		return;
	
	std::unique_lock<std::mutex> lk(mtx);
	redisFree(ctx);
	ctx = nullptr;
}

bool MyRedis::is_open() const
{
	if(!ctx)
		return false;
	
	if(ctx->err)
		return false;
	
	return true;
}

std::vector<MyRedisReply> MyRedis::Query(std::initializer_list<std::string> queries)
{
	return Query(std::vector<std::string>(queries));
}

std::vector<MyRedisReply> MyRedis::Query(std::vector<std::string> queries)
{
	std::vector<MyRedisReply> replies;
	if(!is_open() || queries.size() <= 0)
		return replies;

	std::unique_lock<std::mutex> lk(mtx);

	for(auto &query : queries)
		redisAppendCommand(ctx, query.c_str());
	
	for(auto &query : queries)
	{
		redisReply *reply;
		if(redisGetReply(ctx, (void**)&reply) != REDIS_OK || reply == nullptr)
			return std::vector<MyRedisReply>();
		replies.emplace_back(reply);
		freeReplyObject(reply);
	}

	return replies;
}

ErrorCode MyRedis::SetInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName)
{
	if(!is_open())
		return ErrorCode(ERR_CONNECTION_CLOSED);

	std::string idstr = std::to_string(id);
	std::string cookieStr = Encoder::EncodeBase64(cookie.data(), cookie.size());
	std::vector<std::string> query = {	//기존 세션이 있든 없든, 변경 되든 말든 기록하므로, WATCH를 사용하지 않는다.
		"MULTI",
		"DEL cookie:" + cookieStr,
		"RPUSH cookie:" + cookieStr + " " + idstr + " " + serverName,
		"PEXPIRE cookie:" + cookieStr + " " + timeoutStr,
		"SET account:" + idstr + " " + cookieStr + " PX " + timeoutStr,
		"EXEC"
	};
	std::vector<MyRedisReply> replies = Query(query);
	
	if(replies.size() <= 0)
		return GetErrorCode();

	//추후 WATCH문을 사용해야될 경우가 생기면 EXEC문의 결과 type이 NIL이면 실패한 것으로 간주해야 한다.

	return ErrorCode(SUCCESS);
}

bool MyRedis::RefreshInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName)
{
	if(!is_open())
		return false;

	std::string idstr = std::to_string(id);
	std::string cookieStr = Encoder::EncodeBase64(cookie.data(), cookie.size());

	//Verification
	auto replies = Query({"LRANGE cookie:" + cookieStr + " 0 1"});
	if(replies.size() < 1)
		return false;
	//if(replies[pos].type != REDIS_REPLY_ARRAY)	//Empty Array도 REDIS_REPLY_ARRAY로 온다. 다만 element size가 0일 뿐
	//	return false;
	if(replies[0].elements.size() < 2)
		return false;
	if(replies[0].elements[0].str != idstr)
		return false;
	if(replies[0].elements[1].str != serverName)
		return false;

	//Delete. 중간에 데이터가 변경된 경우, 
	std::vector<std::string> query = {
		"WATCH cookie:" + cookieStr,
		"WATCH account:" + idstr,
		"MULTI",
		"PEXPIRE cookie:" + cookieStr + " " + timeoutStr,
		"PEXPIRE account:" + idstr + " " + timeoutStr,
		"EXEC"
	};
	replies = Query(query);
	if(replies.size() < query.size())
		return false;

	int pos = query.size() - 1;
	if(replies[pos].type != REDIS_REPLY_ARRAY)	//WATCH로 실패 시, REDIS_REPLY_NIL을 반환한다.
		return false;

	if(replies[pos].elements.size() < 2)
		return false;
	if(replies[pos].elements[0].integer < 1)
		return false;
	if(replies[pos].elements[1].integer < 1)
		return false;

	return true;
}

Expected<std::pair<Hash_t, std::string>, ErrorCode> MyRedis::GetInfoFromID(const Account_ID_t& id)
{
	if(!is_open())
		return ErrorCode(ERR_CONNECTION_CLOSED);

	std::string idstr = std::to_string(id);
	Hash_t cookie;
	std::string cookieStr;
	std::string serverName;

	auto replies = Query({"GET account:" + idstr});
	if(replies.size() < 1)
		return GetErrorCode();
	if(replies[0].type != REDIS_REPLY_STRING)
		return ErrorCode(ERR_NO_MATCH_ACCOUNT);
	
	cookieStr = replies[0].str;
	auto vec = Encoder::DecodeBase64(cookieStr);
	std::copy_n(vec.begin(), cookie.size(), cookie.begin());

	std::vector<std::string> query = {
		"MULTI",
		"PEXPIRE account:" + idstr + " " + timeoutStr,
		"PEXPIRE cookie:" + cookieStr + " " + timeoutStr,
		"EXEC",
		"LRANGE cookie:" + cookieStr + " 0 1"
	};
	replies = Query(query);
	if(replies.size() < query.size())
		return GetErrorCode();
	int pos = query.size() - 1;
	if(replies[pos].elements.size() < 2)
		return ErrorCode(ERR_NO_MATCH_ACCOUNT);
	serverName = replies[pos].elements[1].str;

	return std::make_pair(cookie, serverName);
}

Expected<std::pair<Account_ID_t, std::string>, ErrorCode> MyRedis::GetInfoFromCookie(const Hash_t& cookie)
{
	if(!is_open())
		return ErrorCode(ERR_CONNECTION_CLOSED);

	std::string idstr;
	std::string serverName;
	std::string cookieStr = Encoder::EncodeBase64(cookie.begin(), cookie.size());

	auto replies = Query({"LRANGE cookie:" + cookieStr + " 0 1"});
	if(replies.size() < 1)
		return GetErrorCode();
	if(replies[0].type != REDIS_REPLY_ARRAY || replies[0].elements.size() < 2)
		return ErrorCode(ERR_NO_MATCH_ACCOUNT);
	idstr = replies[0].elements[0].str;
	//serverName = replies[0].elements[1].str;

	std::vector<std::string> query = {
		"MULTI",
		"PEXPIRE account:" + idstr + " " + timeoutStr,
		"PEXPIRE cookie:" + cookieStr + " " + timeoutStr,
		"EXEC",
		"LRANGE cookie:" + cookieStr + " 0 1"
	};
	replies = Query(query);
	if(replies.size() < query.size())
		return GetErrorCode();
	
	//세션 타임아웃 갱신 중 값이 변경되거나 세션이 만료되는 경우를 대비하여 새로 값을 받는다.
	int pos = query.size() - 1;
	if(replies[pos].type != REDIS_REPLY_ARRAY || replies[pos].elements.size() < 2)
		return ErrorCode(ERR_NO_MATCH_ACCOUNT);
	idstr = replies[pos].elements[0].str;
	serverName = replies[pos].elements[1].str;

	return std::make_pair(std::stoull(idstr), serverName);
}

bool MyRedis::ClearInfo(const Account_ID_t& id, const Hash_t& cookie, std::string serverName)
{
	if(!is_open())
		return false;
	
	std::string idstr = std::to_string(id);
	std::string cookieStr = Encoder::EncodeBase64(cookie.data(), cookie.size());

	//Verification
	auto replies = Query({"LRANGE cookie:" + cookieStr + " 0 1"});
	if(replies.size() < 1)
		return false;
	//if(replies[pos].type != REDIS_REPLY_ARRAY)	//Empty Array도 REDIS_REPLY_ARRAY로 온다. 다만 element size가 0일 뿐
	//	return false;
	if(replies[0].elements.size() < 2)
		return false;
	if(replies[0].elements[0].str != idstr)
		return false;
	if(replies[0].elements[1].str != serverName)
		return false;

	//Delete. 중간에 데이터가 변경된 경우, 
	std::vector<std::string> query = {
		"WATCH cookie:" + cookieStr,
		"WATCH account:" + idstr,
		"MULTI",
		"DEL cookie:" + cookieStr,
		"DEL account:" + idstr,
		"EXEC"
	};
	replies = Query(query);
	if(replies.size() < query.size())
		return false;

	int pos = query.size() - 1;
	if(replies[pos].type != REDIS_REPLY_ARRAY)	//WATCH로 실패 시, REDIS_REPLY_NIL을 반환한다.
		return false;

	if(replies[pos].elements.size() < 2)
		return false;
	if(replies[pos].elements[0].integer < 1)
		return false;
	if(replies[pos].elements[1].integer < 1)
		return false;

	return true;
}