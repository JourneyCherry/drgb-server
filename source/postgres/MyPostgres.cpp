#include "MyPostgres.hpp"

bool MyPostgres::isRunning = false;
std::unique_ptr<pqxx::connection> MyPostgres::db_c = nullptr;

MyPostgres::MyPostgres()
{
	if(!isRunning)
		throw MyExcepts("Postgres is not running", __STACKINFO__);
	db_w = std::make_unique<pqxx::work>(*db_c);
}

MyPostgres::MyPostgres(MyPostgres&& other)
{
	(*this) = std::move(other);
}

MyPostgres& MyPostgres::operator=(MyPostgres&& other)
{
	isRunning = other.isRunning;
	db_c = std::move(other.db_c);
	return *this;
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
	int retry = MyConfigParser::GetInt("DBRetryCount", 5);
	int wait_sec = MyConfigParser::GetInt("DBRetryDelaySec", 1);
	std::string hostname = MyConfigParser::GetString("DBAddr");
	std::string dbname = MyConfigParser::GetString("DBName");
	int dbport = MyConfigParser::GetInt("DBPort", 5432);
	std::string dbuser = MyConfigParser::GetString("DBUser");
	std::string dbpwd = MyConfigParser::GetString("DBPwd");
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
			MyLogger::log("DB Connection Failed. Wait for " + std::to_string(wait_sec) + " seconds and Retry...(Remain Try : " + std::to_string(retry) + ")", MyLogger::LogType::info);
			std::this_thread::sleep_for(std::chrono::seconds(wait_sec));
			tempe = std::current_exception();
			if(retry > 0)	retry--;	//retry가 음수면 연결이 될 때 까지 시도한다.
			continue;
		}
		break;
	}

	if(db_c == nullptr || !db_c->is_open())
		std::rethrow_exception(tempe);
	MyLogger::log("DB Connection Success", MyLogger::LogType::info);
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

pqxx::result MyPostgres::exec(const std::string& query)
{
	return db_w->exec(query);
}

pqxx::row MyPostgres::exec1(const std::string& query)
{
	return db_w->exec1(query);
}