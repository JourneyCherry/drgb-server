#include "ErrorCode.hpp"

namespace mylib{
namespace utils{

const std::map<errorcode_t, std::string> ErrorCode::strerrcode = {
	{SUCCESS, "Success"},
	{ERR_PROTOCOL_VIOLATION, "Protocol Violation"},
	{ERR_NO_MATCH_ACCOUNT, "There is no matched account"},
	{ERR_EXIST_ACCOUNT, " There is already existing account"},
	{ERR_EXIST_ACCOUNT_MATCH, "There is already existing account in Match Server"},
	{ERR_EXIST_ACCOUNT_BATTLE, "There is already existing account in Battle Server"},
	{ERR_OUT_OF_CAPACITY, "The Server is Out of Capacity"},
	{ERR_DB_FAILED, "DB Fail"},
	{ERR_DUPLICATED_ACCESS, "There is another Access already connected"},
	{ERR_CONNECTION_CLOSED, "Connection is Closed"},
	{ERR_TIMEOUT, "Time out"}
};

ErrorCode::ErrorCode() : ErrorCode(SUCCESS) {}

ErrorCode::ErrorCode(errorcode_t ec) : m_type(TYPE_CUSTOM), m_type_str("CustomCode")
{
	m_code = ec;
	auto msg = strerrcode.find(ec);
	if(msg == strerrcode.end())
		m_message = "Unknown Code";
	else
		m_message = msg->second;
}

ErrorCode::ErrorCode(int _errno) : m_type(TYPE_ERRNO), m_type_str("Errno")
{
	m_code = _errno;
	m_message = std::strerror(_errno);
}

ErrorCode::ErrorCode(boost::system::error_code ec) : m_type(TYPE_BOOST), m_type_str("Boost")
{
	m_code = ec.value();
	m_message = ec.message();
}

ErrorCode::ErrorCode(unsigned long ec) : m_type(TYPE_OPENSSL), m_type_str("OpenSSL")
{
	m_code = ec;
	char *str = ERR_error_string(ec, nullptr);
	size_t len = std::strlen(str);
	m_message = std::string(str, len);
}

ErrorCode::ErrorCode(const ErrorCode& copy)
{
	(*this) = copy;
}

void ErrorCode::SetMessage(std::string new_message)
{
	m_message = "(" + new_message + ")";
}

int ErrorCode::code() const { return m_code; }
std::string ErrorCode::message() const { return m_message; }
std::string ErrorCode::message_code() const { return "(" + std::to_string(m_code) + ") " + m_message; }
std::string ErrorCode::typestr() const { return m_type_str; }
int ErrorCode::typecode() const { return m_type; }

ErrorCode::operator bool() const { return isSuccessed(); }
ErrorCode &ErrorCode::operator=(const ErrorCode& copy)
{
	m_code = copy.m_code;
	m_type = copy.m_type;
	m_type_str = copy.m_type_str;
	m_message = copy.m_message;

	return *this;
}

bool ErrorCode::isSuccessed() const
{
	return m_code == 0;
}

}
}