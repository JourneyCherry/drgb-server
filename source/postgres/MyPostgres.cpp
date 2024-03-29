#include "MyPostgres.hpp"

MyPostgres::MyPostgres(pqxx::connection* conn, std::function<void()> rel_func) : releaseConnection(rel_func)
{
	work = std::make_unique<pqxx::work>(*conn);
}

MyPostgres::~MyPostgres()
{
	//work->abort();	//work가 commit 또는 abort로 transaction이 종료되면 재호출 시, Exception을 던진다.
	work.reset();		//단, 별도의 abort를 명시하지 않아도 work 소멸자에서 자동으로 abort를 호출한다.
	releaseConnection();
}

void MyPostgres::abort()
{
	work->abort();
}

void MyPostgres::commit()
{
	work->commit();
}

std::string MyPostgres::quote(std::string str)
{
	return work->quote(str);
}

std::string MyPostgres::quote_name(std::string str)
{
	return work->quote_name(str);
}

std::string MyPostgres::quote_raw(const unsigned char *data, size_t len)
{
	return work->quote_raw(data, len);
}

pqxx::result MyPostgres::exec(const std::string& query)
{
	return work->exec(query);
}

pqxx::row MyPostgres::exec1(const std::string& query)
{
	return work->exec1(query);
}

Account_ID_t MyPostgres::RegisterAccount(std::string email, Pwd_Hash_t pwd)
{
	auto result = work->exec_params1("INSERT INTO userlist (email, pwd_hash) VALUES ($1, $2) RETURNING id, register_time", email, "UnRegistered-temporaryquote");
	Account_ID_t id = result[0].as<Account_ID_t>();
	std::string reg_time = result[1].as<std::string>();	//TODO : string 외에 byte 타입으로 넣는 방법을 찾아보자.

	Hash_t pwd_hash = GetPwdHash(id, pwd, reg_time);

	auto encoded = Encoder::EncodeBase64(pwd_hash.data(), sizeof(pwd_hash));
	work->exec_params0("UPDATE userlist SET pwd_hash = $2 WHERE id = $1", id, encoded);

	return id;
}

Account_ID_t MyPostgres::FindAccount(std::string email, Pwd_Hash_t pwd)
{
	auto result = work->exec_params1("SELECT id, pwd_hash, register_time FROM userlist WHERE email = $1", email);
	Account_ID_t id = result[0].as<Account_ID_t>();
	std::string encoded = result[1].as<std::string>();
	std::string reg_time = result[2].as<std::string>();

	auto decoded = Encoder::DecodeBase64(encoded);
	if(decoded.size() < sizeof(Hash_t))
		throw StackTraceExcept("Hash in DB size doesn't match : " + std::to_string(decoded.size()), __STACKINFO__);
	Hash_t answer;
	std::copy_n(decoded.begin(), sizeof(Hash_t), answer.begin());

	Hash_t pwd_hash = GetPwdHash(id, pwd, reg_time);
	
	return (pwd_hash == answer)?id:0;
}

bool MyPostgres::ChangePwd(Account_ID_t id, Pwd_Hash_t old_pwd, Pwd_Hash_t new_pwd)
{
	auto result = work->exec_params1("SELECT pwd_hash, register_time FROM userlist WHERE id = $1", id);
	std::string encoded = result[0].as<std::string>();
	std::string reg_time = result[1].as<std::string>();

	auto decoded = Encoder::DecodeBase64(encoded);
	if(decoded.size() < sizeof(Hash_t))
		throw StackTraceExcept("Hash in DB size doesn't match : " + std::to_string(decoded.size()), __STACKINFO__);
	Hash_t answer;
	std::copy_n(decoded.begin(), sizeof(Hash_t), answer.begin());

	Hash_t old_pwd_hash = GetPwdHash(id, old_pwd, reg_time);

	if(old_pwd_hash != answer)
		return false;

	Hash_t new_pwd_hash = GetPwdHash(id, new_pwd, reg_time);
	encoded = Encoder::EncodeBase64(new_pwd_hash.data(), sizeof(Hash_t));

	work->exec_params0("UPDATE userlist SET pwd_hash = $2 WHERE id = $1", id, encoded);

	return true;
}

std::tuple<Achievement_ID_t, int, int, int> MyPostgres::GetInfo(Account_ID_t account_id)
{
	auto nick_result = work->exec_params1("SELECT nickname FROM userlist WHERE id = $1", account_id);
	Achievement_ID_t nick = nick_result[0].as<Achievement_ID_t>();
	//auto result = work->exec_params1("SELECT nickname, win_count, draw_count, loose_count FROM userlist WHERE id = $1", account_id);
	auto result = work->exec_params1("SELECT COUNT(CASE WHEN win = $1 AND flags&2 = 0 THEN 1 END), COUNT(CASE WHEN flags&2 > 0 AND flags&1 = 0 THEN 1 END), COUNT(CASE WHEN loose = $1 AND flags&2 = 0 THEN 1 END) FROM battlelog WHERE win = $1 OR loose = $1", account_id);
	int win = result[0].as<int>();
	int draw = result[1].as<int>();
	int loose = result[2].as<int>();

	return { nick, win, draw, loose };
}

std::tuple<int, int> MyPostgres::GetAchieve(Account_ID_t account_id, Achievement_ID_t achieve_id)
{
	auto result = work->exec_params1("SELECT COALESCE(ua.count, 0), a.count FROM achievement a LEFT JOIN user_achievement ua ON ua.achievement_id = a.id AND ua.user_id = (SELECT id FROM userlist WHERE id = $1) WHERE a.id = (SELECT id FROM achievement WHERE id = $2)", account_id, achieve_id);
	return {result[0].as<int>(), result[1].as<int>()};
}

std::map<Achievement_ID_t, int> MyPostgres::GetAllAchieve(Account_ID_t account_id)
{
	auto result = work->exec_params("SELECT a.id, ua.count FROM achievement a INNER JOIN user_achievement ua ON a.id = ua.achievement_id AND ua.user_id = $1", account_id);
	std::map<Achievement_ID_t, int> achievements;
	for(auto row : result)
		achievements.insert({row[0].as<Achievement_ID_t>(), row[1].as<int>()});

	return achievements;
}

bool MyPostgres::SetNickName(Account_ID_t account_id, Achievement_ID_t achieve_id)
{
	auto [achieve, goal] = GetAchieve(account_id, achieve_id);
	if(achieve < goal)
		return false;
	work->exec_params0("UPDATE userlist SET nickname = $2 WHERE id = $1", account_id, achieve_id);
	return true;
}

bool MyPostgres::Achieve(Account_ID_t id, Achievement_ID_t achieve)
{
	auto [count, require] = GetAchieve(id, achieve);
	if(count < require)	//미달성 조건
	{
		work->exec_params0("INSERT INTO user_achievement (user_id, achievement_id, count) VALUES ($1, $2, 1) ON CONFLICT (user_id, achievement_id) DO UPDATE SET count = user_achievement.count + 1 WHERE user_achievement.user_id = $1 AND user_achievement.achievement_id = $2", id, achieve);	//ON CONFLICT는 PostgreSQL에서만 사용가능하다. MySQL은 ON DUPLICATE KEY이다.
		work->commit();
		return (require - count == 1);
	}
	return false;
}

bool MyPostgres::AchieveProgress(Account_ID_t id, Achievement_ID_t achieve, int c)
{
	auto [count, require] = GetAchieve(id, achieve);
	if(count < require && 	//미달성 조건
		count < c)			//진행도가 기존보다 더 진행되어야 함
	{
		c = std::min(require, c);
		work->exec_params0("INSERT INTO user_achievement (user_id, achievement_id, count) VALUES ($1, $2, $3) ON CONFLICT (user_id, achievement_id) DO UPDATE SET count = $3 WHERE user_achievement.user_id = $1 AND user_achievement.achievement_id = $2", id, achieve, c);	//ON CONFLICT는 PostgreSQL에서만 사용가능하다. MySQL은 ON DUPLICATE KEY이다.
		work->commit();
		return (require <= c);	//3줄 위에서 require를 넘지않게 막고 있지만 혹시나 모를 경우를 위해 '동일'이 아닌 '이상'조건으로 설정함.
	}
	return false;
}

void MyPostgres::ArchiveBattle(Account_ID_t winner, Account_ID_t looser, bool draw, bool crashed)
{
	short flags = 0;	//SMALLINT
	flags |= draw?0b10:0;
	flags |= crashed?0b01:0;
	work->exec_params0("INSERT INTO battlelog (win, loose, flags) VALUES ($1, $2, $3)", winner, looser, flags);
}

Hash_t MyPostgres::GetPwdHash(Account_ID_t id, Pwd_Hash_t pwd, std::string reg_time)
{
	static constexpr int MAX_HASH_ITERATION = 17;	//최소 1회 ~ 최대 65536회
	ByteQueue result = ByteQueue::Create<Hash_t>(pwd);
	ByteQueue salt = ByteQueue(reg_time.c_str(), reg_time.size());
	unsigned long long iterateCount = 1 << (id % MAX_HASH_ITERATION);
	for(unsigned long long i = 0;i<iterateCount;i++)
		result = ByteQueue::Create<Hash_t>(Hasher::sha256(result + salt));

	return result.pop<Hash_t>();
}