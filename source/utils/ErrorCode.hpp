#pragma once
#include <map>
#include <string>
#include <cerrno>
#include <boost/asio/error.hpp>
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
		static constexpr int TYPE_CUSTOM = 2;

		ErrorCode();
		ErrorCode(boost::system::error_code);
		ErrorCode(int);
		ErrorCode(errorcode_t);
		int code() const;
		std::string message() const;
		std::string message_code() const;
		std::string typestr() const;
		int typecode() const;
		operator bool() const;
};

}
}