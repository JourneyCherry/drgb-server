#pragma once
#include <functional>

namespace mylib{
namespace utils{

/**
 * @brief Object containing result value on success or error value on failure.(similar with std::variant<T, E> in C++17 and same as std::expected<T, E> in C++23)
 * 
 * @tparam T the type of result value
 * @tparam E the type of error value. 
 */
template <typename T, typename E=void>
class Expected
{
	private:
		bool m_isSuccessed;
		T m_value;
		E m_ec;
	public:
		/**
		 * @brief Constructor of Expected<T, E>
		 * 
		 * @param value value on success
		 * @param isSuccessed whether it has successed or failed.
		 */
		Expected(T value, bool isSuccessed = true) : m_isSuccessed(isSuccessed), m_value(value){}
		/**
		 * @brief Constructor of Expected<T, E> with full value.
		 * 
		 * @param value value on success
		 * @param ec value on failure
		 * @param isSuccessed whether it has successed or failed.
		 */
		Expected(T value, E ec, bool isSuccessed) : m_isSuccessed(isSuccessed), m_value(value), m_ec(ec){}
		/**
		 * @brief Constructor of Expected<T, E> on failure
		 * 
		 * @param ec value on failure
		 */
		Expected(E ec) : m_isSuccessed(false), m_ec(ec){}
		Expected(const Expected& copy){*this = copy;}
		Expected(Expected&& move){*this = std::move(move);}
		~Expected() = default;
		/**
		 * @brief boolean operator.
		 * 
		 * @return true if it has successed.
		 * @return false if it has failed.
		 */
		operator bool(){ return m_isSuccessed;}
		/**
		 * @brief whether it has successed or failed.
		 * 
		 * @return true if it has successed.
		 * @return false if it has failed.
		 */
		bool isSuccessed(){return m_isSuccessed;}
		/**
		 * @brief Get reference pointer of value on success
		 * 
		 * @return T& reference pointer of value on success
		 */
		T& operator*(){return std::ref(m_value);}
		/**
		 * @brief '->' operator on successed value.
		 * 
		 * @return T* pointer of value on success
		 */
		T* operator->(){return &m_value;}
		/**
		 * @brief Get value on success
		 * 
		 * @return T value on success
		 */
		T value(){return m_value;}
		/**
		 * @brief Get value on failure
		 * 
		 * @return E value on failure
		 */
		E error(){return m_ec;}
		/**
		 * @brief Get value on success or default value.
		 * 
		 * @param default_value default value to get on failed state.
		 * @return T result
		 */
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
		/**
		 * @brief comparison operator of Expected<T, E>. Both should have successed and have same value.
		 * 
		 * @param rhs right hands
		 * @return true if both have successed and same value.
		 * @return false if one of them has failed or have different value on success.
		 */
		bool operator==(const Expected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:m_ec == rhs.m_ec;
			return false;
		}
};

/**
 * @brief Object containing result value on success or nothing on failure.(same as std::optional<T> in C++17)
 * 
 * @tparam T type of result value
 */
template <typename T>
class Expected<T, void>
{
	private:
		bool m_isSuccessed;
		T m_value;
	public:
		/**
		 * @brief Constructor of Expected<T>.
		 * 
		 * @param value value on success
		 * @param isSuccess whether it has successed or failed.
		 */
		Expected(T value, bool isSuccess = true) : m_isSuccessed(isSuccess), m_value(value){}
		/**
		 * @brief Constructor of Expected<T> on failure.
		 * 
		 */
		Expected() : m_isSuccessed(false){}
		Expected(const Expected& copy){*this = copy;}
		Expected(Expected&& move){*this = std::move(move);}
		~Expected() = default;
		/**
		 * @brief boolean operator.
		 * 
		 * @return true if it has successed.
		 * @return false if it has failed.
		 */
		operator bool(){ return m_isSuccessed;}
		/**
		 * @brief whether it has successed or failed.
		 * 
		 * @return true if it has successed.
		 * @return false if it has failed.
		 */
		bool isSuccessed(){return m_isSuccessed;}
		/**
		 * @brief Get reference pointer of value on success
		 * 
		 * @return T& reference pointer of value on success
		 */
		T& operator*(){return std::ref(m_value);}
		/**
		 * @brief '->' operator on successed value.
		 * 
		 * @return T* pointer of value on success
		 */
		T* operator->(){return &m_value;}
		/**
		 * @brief Get value on success
		 * 
		 * @return T value on success
		 */
		T value(){return m_value;}
		/**
		 * @brief Get value on success or default value.
		 * 
		 * @param default_value default value to get on failed state.
		 * @return T result
		 */
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
		/**
		 * @brief comparison operator of Expected<T>. Both should have successed and have same value.
		 * 
		 * @param rhs right hands
		 * @return true if both have successed and same value.
		 * @return false if one of them has failed or have different value on success.
		 */
		bool operator==(const Expected& rhs)
		{
			if(m_isSuccessed == rhs.m_isSuccessed)
				return m_isSuccessed?m_value == rhs.m_value:true;
			return false;
		}
};

}
}