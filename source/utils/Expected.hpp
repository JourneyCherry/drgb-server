#pragma once
#include <functional>

namespace mylib{
namespace utils{

template <typename T, typename E=void>
class Expected
{
	private:
		bool m_isSuccessed;
		T m_value;
		E m_ec;
	public:
		Expected(T value, bool isSuccessed = true) : m_isSuccessed(isSuccessed), m_value(value){}
		Expected(T value, E ec, bool isSuccessed) : m_isSuccessed(isSuccessed), m_value(value), m_ec(ec){}
		Expected(E ec) : m_isSuccessed(false), m_ec(ec){}
		Expected(const Expected& copy){*this = copy;}
		Expected(Expected&& move){*this = std::move(move);}
		~Expected() = default;
		operator bool(){ return m_isSuccessed;}
		bool isSuccessed(){return m_isSuccessed;}
		T& operator*(){return std::ref(m_value);}
		T* operator->(){return &m_value;}
		T value(){return m_value;}
		E error(){return m_ec;}
		T value_or(T default_value){return m_isSuccessed?m_value:default_value;}
		Expected& operator=(const Expected& copy) noexcept
		{
			m_isSuccessed = copy.m_isSuccessed;
			m_value = copy.m_value;
			m_ec = copy.m_ec;

			return *this;
		}
		Expected& operator=(Expected&& move) noexcept
		{
			m_isSuccessed = std::move(move.m_isSuccessed);
			m_value = std::move(move.m_value);
			m_ec = std::move(move.m_ec);

			return *this;
		}
		bool operator==(const Expected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:m_ec == rhs.m_ec;
			return false;
		}
};


template <typename T>
class Expected<T, void>
{
	private:
		bool m_isSuccessed;
		T m_value;
	public:
		Expected(T value, bool isSuccess = true) : m_isSuccessed(isSuccess), m_value(value){}
		Expected() : m_isSuccessed(false){}
		Expected(const Expected& copy){*this = copy;}
		Expected(Expected&& move){*this = std::move(move);}
		~Expected() = default;
		operator bool(){ return m_isSuccessed;}
		bool isSuccessed(){return m_isSuccessed;}
		T& operator*(){return std::ref(m_value);}
		T* operator->(){return &m_value;}
		T value(){return m_value;}
		T value_or(T dvalue){return m_isSuccessed?m_value:dvalue;}
		Expected& operator=(const Expected& copy) noexcept
		{
			m_isSuccessed = copy.m_isSuccessed;
			m_value = copy.m_value;

			return *this;
		}
		Expected& operator=(Expected&& move) noexcept
		{
			m_isSuccessed = std::move(move.m_isSuccessed);
			m_value = std::move(move.m_value);

			return *this;
		}
		bool operator==(const Expected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:true;
			return false;
		}
};

}
}