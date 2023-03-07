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