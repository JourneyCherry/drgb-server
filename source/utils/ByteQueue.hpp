#pragma once
#include <cstring> //For memcpy() in cpp
#include <vector>
#include <functional>
#include <stdexcept>
#include "MyTypes.hpp"

namespace mylib{
namespace utils{

/**
 * @brief Byte Vector for any data types.
 * 
 */
class ByteQueue
{
	private:
		static bool g_isLittleEndian;
		/**
		 * @brief Get default Endian of the system.
		 * 
		 * @return true if the system is on Little Endian.
		 * @return false if the system is on Big Endian.
		 */
		static bool isLittleEndianSystem();
		bool defaultEndian;
		std::vector<byte> bytes;
		long unsigned int ptr;
	public:
		/**
		 * @brief Construct a new empty ByteQueue object.
		 * 
		 */
		ByteQueue();
		/**
		 * @brief Construct a new ByteQueue object with byte stream.
		 * 
		 * @param bstream series of bytes.
		 */
		ByteQueue(std::vector<byte> bstream);
		/**
		 * @brief Construct a new ByteQueue object with endian
		 * 
		 * @param endian set true for Little Endian or false for Big Endian.
		 */
		ByteQueue(bool endian);
		/**
		 * @brief Construct a new ByteQueue object with memory data.
		 * 
		 * @param buf target buffer in memory
		 * @param size size of the buffer
		 */
		ByteQueue(const char *buf, size_t size);
		ByteQueue(const ByteQueue& copy);	//Copy Constructor
		ByteQueue(ByteQueue&& move) noexcept;	//Move Constructor
		ByteQueue &operator=(const ByteQueue& copy);	//Assignment Operator
		ByteQueue& operator=(ByteQueue&& move);		//Move Assignment Operator
		/**
		 * @brief Addition Operator of ByteQueue. rhs(Right Hand Side) will be added onto the back of the lhs(Left Hand Side) object.
		 * 
		 * @param adder target object for addition.
		 * @return ByteQueue result of 'lhs + rhs'
		 */
		ByteQueue operator+(const ByteQueue& adder) const;
		/**
		 * @brief Get data on 'index'th location.
		 * 
		 * @param index location of required data.
		 * @return byte data on 'index'th location.
		 * 
		 * @throw std::out_of_range if index is out of ranged.
		 */
		byte operator[](long unsigned int index) const;
		/**
		 * @brief Addition Assignment Operator of ByteQueue. 'adder' will be added onto the back of the original object.
		 * 
		 * @param adder target object for addition.
		 * @return ByteQueue& result of 'this += adder'
		 */
		ByteQueue& operator+=(const ByteQueue& adder);
		~ByteQueue();
		/**
		 * @brief Add data onto back of the original data with system endian.
		 * 
		 * @tparam T Type of the data to add
		 * @param data data to add
		 */
		template <typename T> void push(T data);
		/**
		 * @brief Add data onto back of the original data.
		 * 
		 * @tparam T Type of the data to add
		 * @param data data to add
		 * @param endian true for Little Endian, false for Big Endian.
		 */
		template <typename T> void push(T data, bool endian);
		/**
		 * @brief Add data onto front of the original data with system endian.
		 * 
		 * @tparam T Type of the data to add
		 * @param data data to add
		 */
		template <typename T> void push_head(T data);
		/**
		 * @brief Add data onto front of the original data.
		 * 
		 * @tparam T Type of the data to add
		 * @param data data to add
		 * @param endian true for Little Endian, false for Big Endian.
		 */
		template <typename T> void push_head(T data, bool endian);
		/**
		 * @brief Add raw data onto back of the original data.
		 * 
		 * @param datas pointer of the data.
		 * @param len size of the data.
		 */
		void pushraw(const byte* datas, size_t len);
		/**
		 * @brief Remove and Return the first data from the original data with system endian.
		 * 
		 * @tparam T Type of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @return T the first data.
		 */
		template <typename T> T pop(bool peek = false);
		/**
		 * @brief Remove and Return the first data from the original data.
		 * 
		 * @tparam T Type of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @param endian true for Little Endian, false for Big Endian.
		 * @return T the first data.
		 */
		template <typename T> T pop(bool peek, bool endian);
		/**
		 * @brief Remove and Return all of the original data with system endian.
		 * 
		 * @tparam T Type of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @return std::vector<T> series of the whole data in type T.
		 */
		template <typename T> std::vector<T> pops(bool peek = false);
		/**
		 * @brief Remove and Return all of the original data.
		 * 
		 * @tparam T Type of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @param endian true for Little Endian, false for Big Endian.
		 * @return std::vector<T> series of the whole data in type T.
		 */
		template <typename T> std::vector<T> pops(bool peek, bool endian);
		/**
		 * @brief Remove and Return some of the original data from front of it with system endian.
		 * 
		 * @tparam T Type of the data to get.
		 * @param size the number of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @return std::vector<T> series of the data in type T.
		 */
		template <typename T> std::vector<T> pops(size_t size, bool peek = false);
		/**
		 * @brief Remove and Return some of the original data from front of it.
		 * 
		 * @tparam T Type of the data to get.
		 * @param size the number of the data to get.
		 * @param peek true if you don't want to remove. Default is false.
		 * @param endian true for Little Endian, false for Big Endian.
		 * @return std::vector<T> series of the data in type T.
		 */
		template <typename T> std::vector<T> pops(size_t size, bool peek, bool endian);
		/**
		 * @brief Remove and Return string data from current to end.
		 * 
		 * @param peek true if you don't want to remove. Default is false.
		 * @return std::string string data.
		 */
		std::string popstr(bool peek = false);
		/**
		 * @brief Remove and Return string data from current for 'len' size.
		 * 
		 * @param len length for data
		 * @param peek true if you don't want to remove. Default is false.
		 * @return std::string string data.
		 */
		std::string popstr(size_t len, bool peek = false);
		/**
		 * @brief Set endian type for this ByteQueue object.
		 * 
		 * @param endian set true for Little Endian or false for Big Endian.
		 */
		void SetEndian(bool endian);
		/**
		 * @brief Move pointer from current to next 'len'th data.
		 * 
		 * @param len length for pointer.
		 * @throw std::out_of_range thrown if 'current + len' exceed the size of the data.
		 */
		void Next(int len);
		/**
		 * @brief Move pointer from current to previous 'len'th data. 
		 * 
		 * @param len length for pointer. if 'current - len' become less than 0, it will be set by 0(first data).
		 */
		void Prev(int len);
		/**
		 * @brief Reset pointer to 0(first data).
		 * 
		 */
		void Reset();
		/**
		 * @brief Remove all data from this ByteQueue object.
		 * 
		 */
		void Clear();
		/**
		 * @brief Get total size of data.
		 * 
		 * @return size_t bytes of all data.
		 */
		size_t Size() const;
		/**
		 * @brief Get size of data from current pointer to end.
		 * 
		 * @return size_t bytes of remain(popable) data.
		 */
		size_t Remain();
		/**
		 * @brief Get byte pointer of origin data in this ByteQueue object.
		 * 
		 * @return const byte* byte pointer of origin data
		 */
		const byte* data();
		/**
		 * @brief Get Vector STL Container of origin data in this ByteQueue object.
		 * 
		 * @return const std::vector<byte> Vector Container of origin data
		 */
		const std::vector<byte> vector();
		/**
		 * @brief Create a new ByteQueue object with a T type of data contained.
		 * 
		 * @tparam T type of initial data
		 * @param data initial data
		 * @return ByteQueue new object with a data
		 */
		template<typename T> static ByteQueue Create(T data);
		/**
		 * @brief Get a new ByteQueue object with a partial data of original object from 'start'th to 'end'th data.
		 * 
		 * @param start start position of the part
		 * @param end end position of the part
		 * @return ByteQueue new object with a partial data
		 */
		ByteQueue split(long long int start = -1, long long int end = -1) const;
};

//Specialization. Implementation is in .cpp file.
template <>
void ByteQueue::push<std::string>(std::string);

template <>
ByteQueue ByteQueue::Create<std::string>(std::string);
////

template <typename T>
void ByteQueue::push(T data)
{
	push<T>(data, defaultEndian);
}

template <typename T>
void ByteQueue::push(T data, bool endian)
{
	byte *p = (byte*)&data;
	unsigned int len = sizeof(T) / sizeof(byte);
	std::function<byte(unsigned int)> value;
	if(g_isLittleEndian == endian)
		value = [p](int i){return p[i];};
	else
		value = [p, len](int i){return p[len - i - 1];};
	for(unsigned int i = 0;i<len;i++)
		bytes.push_back(value(i));
}

template <typename T>
void ByteQueue::push_head(T data)
{
	push_head<T>(data, defaultEndian);
}

template <typename T>
void ByteQueue::push_head(T data, bool endian)
{
	byte *p = (byte*)&data;
	unsigned int len = sizeof(T) / sizeof(byte);
	std::function<byte(unsigned int)> value;
	if(g_isLittleEndian == endian)
		value = [p](int i){return p[i];};
	else
		value = [p, len](int i){return p[len - i - 1];};
	for(unsigned int i = 0;i<len;i++)
		bytes.insert(bytes.begin() + i, value(i));
}

template <typename T>
std::vector<T> ByteQueue::pops(bool peek)
{
	return pops<T>(Remain(), peek, defaultEndian);
}

template <typename T>
std::vector<T> ByteQueue::pops(bool peek, bool endian)
{
	return pops<T>(Remain(), peek, endian);
}

template <typename T>
std::vector<T> ByteQueue::pops(size_t size, bool peek)
{
	return pops<T>(size, peek, defaultEndian);
}

template <typename T>
std::vector<T> ByteQueue::pops(size_t size, bool peek, bool endian)
{
	if(size <= 0)
		size = Remain() - ptr;

	size_t cell = sizeof(T);
	if(ptr + cell * size > Remain())
		throw std::out_of_range("ByteQueue : Byte Out of Range");

	std::vector<T> result;
	
	result.reserve(size);
	for(int i = 0;i<size;i++)
	{
		std::vector<byte> bstream;
		T value;
		bstream.reserve(cell);
		if(g_isLittleEndian == endian)
		{
			for(int j = 0;j<cell;j++)
				bstream.push_back(bytes[ptr + i*cell + j]);
		}
		else
		{
			for(int j = cell - 1;j >= 0;j--)
				bstream.push_back(bytes[ptr + i*cell + j]);
		}
		memcpy(&value, bstream.data(), cell);
		result.push_back(value);
	}

	if(!peek)
		ptr += cell*size;

	return result;
}

template <typename T>
T ByteQueue::pop(bool peek)
{
	return pop<T>(peek, defaultEndian);
}

template <typename T>
T ByteQueue::pop(bool peek, bool endian)
{
	unsigned int len = sizeof(T) / sizeof(byte);
	if(ptr + len > bytes.size())
		throw std::out_of_range("ByteQueue : Byte Out of Range");

	T result;
	std::vector<byte> b;
	std::function<byte(int, int, int)> value;
	if(g_isLittleEndian == endian)
		value = [this](int start, int size, int i){return bytes[start + i];};
	else
		value = [this](int start, int size, int i){return bytes[start + size - i - 1];};

	for(unsigned int i = 0;i<len;i++)
		b.push_back(value(ptr, len, i));

	if(!peek)
		ptr += len;

	memcpy(&result, b.data(), len);

	return result;
}

template <typename T>
ByteQueue ByteQueue::Create(T data)
{
	ByteQueue result;
	result.push<T>(data);

	return result;
}

}
}