#pragma once
#include <map>
#include <string>
#include <cerrno>
#include <boost/asio/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <openssl/err.h>
#include "MyCodes.hpp"

namespace mylib{
namespace utils{

class ErrorCode
{
	private:
		static const std::map<errorcode_t, std::string> strerrcode;
		
	private:
		int m_code;
		int m_type;
		std::string m_type_str;
		std::string m_message;

	public:
		static constexpr int TYPE_ERRNO = 0;
		static constexpr int TYPE_BOOST = 1;
		static constexpr int TYPE_OPENSSL = 2;
		static constexpr int TYPE_CUSTOM = 3;

		ErrorCode();
		ErrorCode(boost::system::error_code);
		ErrorCode(int);
		ErrorCode(errorcode_t);
		ErrorCode(unsigned long);
		ErrorCode(const ErrorCode&);
		void SetMessage(std::string);
		int code() const;
		std::string message() const;
		std::string message_code() const;
		std::string typestr() const;
		int typecode() const;
		ErrorCode &operator=(const ErrorCode&);
		operator bool() const;
		bool isSuccessed() const;
};

}
}