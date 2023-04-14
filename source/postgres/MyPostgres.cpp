#include "MyPostgres.hpp"

bool MyPostgres::isRunning = false;
std::unique_ptr<pqxx::connection> MyPostgres::db_c = nullptr;
std::mutex MyPostgres::db_mtx;

MyPostgres::MyPostgres() : db_lk(db_mtx)	//TODO : pqxx::connection 한개당 pqxx::work(즉, transaction)은 동시에 1개만 가능하다.
{											//		 즉, connection은 재사용가능하지만 transaction 자체는 single thread에서만 접근해야 한다.
	if(!isRunning)							//		 multi-thread query를 하려면 connection pool을 만들자.
		throw StackTraceExcept("Postgres is not running", __STACKINFO__);
	db_w = std::make_unique<pqxx::work>(*db_c);
}

MyPostgres::~MyPostgres()
{
	//db_w->abort();	//work가 commit 또는 abort로 transaction이 종료되면 재호출 시, Exception을 던진다.
	db_w.reset();		//단, 별도의 abort를 명시하지 않아도 work 소멸자에서 자동으로 abort를 호출한다.
}

bool MyPostgres::isOpened()
{
	return isRunning && (db_c?db_c->is_open():false);
}

void MyPostgres::Open()
{
	int retry = ConfigParser::GetInt("DBRetryCount", 5);
	int wait_sec = ConfigParser::GetInt("DBRetryDelaySec", 1);
	std::string hostname = ConfigParser::GetString("DBAddr");
	std::string dbname = ConfigParser::GetString("DBName");
	int dbport = ConfigParser::GetInt("DBPort", 5432);
	std::string dbuser = ConfigParser::GetString("DBUser");
	std::string dbpwd = ConfigParser::GetString("DBPwd");
	std::string db_conn_str = "host=" + hostname + " dbname=" + dbname + " user=" + dbuser + " password=" + dbpwd + " port=" + std::to_string(dbport);

	std::exception_ptr tempe;

	while(retry != 0)
	{
		try
		{
			db_c = std::make_unique<pqxx::connection>(db_conn_str);	//예외가 발생하면 main으로 던진다.
		}
		catch(pqxx::failure e)
		{
			Logger::log("DB Connection Failed. Wait for " + std::to_string(wait_sec) + " seconds and Retry...(Remain Try : " + std::to_string(retry) + ")", Logger::LogType::info);
			std::this_thread::sleep_for(std::chrono::seconds(wait_sec));
			tempe = std::current_exception();
			if(retry > 0)	retry--;	//retry가 음수면 연결이 될 때 까지 시도한다.
			continue;
		}
		break;
	}

	if(db_c == nullptr || !db_c->is_open())
		std::rethrow_exception(tempe);
	Logger::log("DB Connection Success", Logger::LogType::info);
	isRunning = true;
}

void MyPostgres::Close()
{
	if(isRunning)
	{
		db_c.reset();
		isRunning = false;
	}
}

void MyPostgres::abort()
{
	db_w->abort();
}

void MyPostgres::commit()
{
	db_w->commit();
}

std::string MyPostgres::quote(std::string str)
{
	return db_w->quote(str);
}

std::string MyPostgres::quote_name(std::string str)
{
	return db_w->quote_name(str);
}

std::string MyPostgres::quote_raw(const unsigned char *data, size_t len)
{
	return db_w->quote_raw(data, len);
}

pqxx::result MyPostgres::exec(const std::string& query)
{
	return db_w->exec(query);
}

pqxx::row MyPostgres::exec1(const std::string& query)
{
	return db_w->exec1(query);
}

Account_ID_t MyPostgres::RegisterAccount(std::string email, Pwd_Hash_t pwd)
{
	auto result = db_w->exec_params1("INSERT INTO userlist (email, pwd_hash) VALUES ($1, $2) RETURNING id, register_time", email, "UnRegistered-temporaryquote");
	Account_ID_t id = result[0].as<Account_ID_t>();
	std::string reg_time = result[1].as<std::string>();	//TODO : string 외에 byte 타입으로 넣는 방법을 찾아보자.

	Hash_t pwd_hash = GetPwdHash(id, pwd, reg_time);
	
	auto encoded = Encoder::EncodeBase64(pwd_hash.data(), sizeof(pwd_hash));
	db_w->exec_params0("UPDATE userlist SET pwd_hash = $2 WHERE id = $1", id, encoded);

	return id;
}

Account_ID_t MyPostgres::FindAccount(std::string email, Pwd_Hash_t pwd)
{
	auto result = db_w->exec_params1("SELECT id, pwd_hash, register_time FROM userlist WHERE email = $1", email);
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

std::tuple<Achievement_ID_t, int, int, int> MyPostgres::GetInfo(Account_ID_t account_id)
{
	auto result = db_w->exec_params1("SELECT nickname, win_count, draw_count, loose_count FROM userlist WHERE id = $1", account_id);
	return {
		result[0].as<Achievement_ID_t>(), 
		result[1].as<int>(), 
		result[2].as<int>(), 
		result[3].as<int>()
	};
}

std::tuple<int, int> MyPostgres::GetAchieve(Account_ID_t account_id, Achievement_ID_t achieve_id)
{
	auto result = db_w->exec_params1("SELECT COALESCE(ua.count, 0), a.count FROM achievement a LEFT JOIN user_achievement ua ON ua.achievement_id = a.id AND ua.user_id = (SELECT id FROM userlist WHERE id = $1) WHERE a.id = (SELECT id FROM achievement WHERE id = $2)", account_id, achieve_id);
	return {result[0].as<int>(), result[1].as<int>()};
}

std::map<Achievement_ID_t, int> MyPostgres::GetAllAchieve(Account_ID_t account_id)
{
	auto result = db_w->exec_params("SELECT a.id, ua.count FROM achievement a INNER JOIN user_achievement ua ON a.id = ua.achievement_id AND ua.user_id = $1", account_id);
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
	db_w->exec_params0("UPDATE userlist SET nickname = $2 WHERE id = $1", account_id, achieve_id);
	return true;
}

void MyPostgres::IncreaseScore(Account_ID_t id, int result)
{
	std::string query = "UPDATE userlist SET ";
	if(result > 0)
		query += "win_count = win_count + 1 ";
	else if(result < 0)
		query += "loose_count = loose_count + 1 ";
	else
		query += "draw_count = draw_count + 1 ";
	query += "WHERE id = $1";
	db_w->exec_params0(query.c_str(), id);
}

bool MyPostgres::Achieve(Account_ID_t id, Achievement_ID_t achieve)
{
	auto [count, require] = GetAchieve(id, achieve);
	if(count < require)
	{
		db_w->exec_params0("INSERT INTO user_achievement (user_id, achievement_id, count) VALUES ($1, $2, 1) ON CONFLICT (user_id, achievement_id) DO UPDATE SET count = user_achievement.count + 1 WHERE user_achievement.user_id = $1 AND user_achievement.achievement_id = $2", id, achieve);	//ON CONFLICT는 PostgreSQL에서만 사용가능하다. MySQL은 ON DUPLICATE KEY이다.
		db_w->commit();
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
		db_w->exec_params0("INSERT INTO user_achievement (user_id, achievement_id, count) VALUES ($1, $2, $3) ON CONFLICT (user_id, achievement_id) DO UPDATE SET count = $3 WHERE user_achievement.user_id = $1 AND user_achievement.achievement_id = $2", id, achieve, c);	//ON CONFLICT는 PostgreSQL에서만 사용가능하다. MySQL은 ON DUPLICATE KEY이다.
		db_w->commit();
		return (require <= c);	//3줄 위에서 require를 넘지않게 막고 있지만 혹시나 모를 경우를 위해 '동일'이 아닌 '이상'조건으로 설정함.
	}
	return false;
}

Hash_t GetPwdHash(Account_ID_t id, Pwd_Hash_t pwd, std::string reg_time)
{
	static constexpr int MAX_HASH_ITERATION = 32;
	ByteQueue result = ByteQueue::Create<Hash_t>(pwd);
	ByteQueue salt = ByteQueue(reg_time.c_str(), reg_time.size());
	unsigned long long iterateCount = 1 << (id % MAX_HASH_ITERATION);
	for(unsigned long long i = 0;i<iterateCount;i++)
		result = ByteQueue::Create<Hash_t>(Hasher::sha256(result + salt));

	return result.pop<Hash_t>();
}