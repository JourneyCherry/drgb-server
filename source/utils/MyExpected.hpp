#pragma once
#include <functional>

template <typename T, typename E=void>
class MyExpected
{
	private:
		bool m_isSuccessed;
		T m_value;
		E m_ec;
	public:
		MyExpected(T value, bool isSuccessed = true) : m_isSuccessed(isSuccessed), m_value(value){}
		MyExpected(T value, E ec, bool isSuccessed) : m_isSuccessed(isSuccessed), m_value(value), m_ec(ec){}
		MyExpected(E ec) : m_isSuccessed(false), m_ec(ec){}
		MyExpected(const MyExpected& copy){*this = copy;}
		MyExpected(MyExpected&& move){*this = std::move(move);}
		~MyExpected() = default;
		operator bool(){ return m_isSuccessed;}
		bool isSuccessed(){return m_isSuccessed;}
		T& operator*(){return std::ref(m_value);}
		T* operator->(){return &m_value;}
		T value(){return m_value;}
		E error(){return m_ec;}
		T value_or(T default_value){return m_isSuccessed?m_value:default_value;}
		MyExpected& operator=(const MyExpected& copy) noexcept
		{
			m_isSuccessed = copy.m_isSuccessed;
			m_value = copy.m_value;
			m_ec = copy.m_ec;

			return *this;
		}
		MyExpected& operator=(MyExpected&& move) noexcept
		{
			m_isSuccessed = std::move(move.m_isSuccessed);
			m_value = std::move(move.m_value);
			m_ec = std::move(move.m_ec);

			return *this;
		}
		bool operator==(const MyExpected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:m_ec == rhs.m_ec;
			return false;
		}
};


template <typename T>
class MyExpected<T, void>
{
	private:
		bool m_isSuccessed;
		T m_value;
	public:
		MyExpected(T value, bool isSuccess = true) : m_isSuccessed(isSuccess), m_value(value){}
		MyExpected() : m_isSuccessed(false){}
		MyExpected(const MyExpected& copy){*this = copy;}
		MyExpected(MyExpected&& move){*this = std::move(move);}
		~MyExpected() = default;
		operator bool(){ return m_isSuccessed;}
		bool isSuccessed(){return m_isSuccessed;}
		T& operator*(){return std::ref(m_value);}
		T* operator->(){return &m_value;}
		T value(){return m_value;}
		T value_or(T dvalue){return m_isSuccessed?m_value:dvalue;}
		MyExpected& operator=(const MyExpected& copy) noexcept
		{
			m_isSuccessed = copy.m_isSuccessed;
			m_value = copy.m_value;

			return *this;
		}
		MyExpected& operator=(MyExpected&& move) noexcept
		{
			m_isSuccessed = std::move(move.m_isSuccessed);
			m_value = std::move(move.m_value);

			return *this;
		}
		bool operator==(const MyExpected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:true;
			return false;
		}
};