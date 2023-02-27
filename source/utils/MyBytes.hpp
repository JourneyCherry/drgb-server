#pragma once
#include <cstring> //For memcpy() in cpp
#include <vector>
#include <functional>
#include <stdexcept>
#include "MyTypes.hpp"

class MyBytes
{
	private:
		static bool g_isLittleEndian;
		static bool isLittleEndianSystem();
		bool defaultEndian;
		std::vector<byte> bytes;
		long unsigned int ptr;
	public:
		MyBytes();
		MyBytes(std::vector<byte>);
		MyBytes(bool);
		MyBytes(const MyBytes&);
		MyBytes(const char *, size_t);
		MyBytes(MyBytes&&) noexcept;
		MyBytes &operator=(const MyBytes&);	//대입 연산자
		MyBytes& operator=(MyBytes&&);		//이동 대입 연산자
		MyBytes operator+(const MyBytes&) const;
		byte operator[](long unsigned int) const;
		MyBytes& operator+=(const MyBytes&);
		~MyBytes();
		template <typename T> void push(T);
		template <typename T> void push(T, bool);
		template <typename T> void push_head(T);
		template <typename T> void push_head(T, bool);
		void push(const byte*, int);
		template <typename T> T pop(bool = false);
		template <typename T> T pop(bool, bool);
		template <typename T> std::vector<T> pops(bool = false);
		template <typename T> std::vector<T> pops(bool, bool);
		template <typename T> std::vector<T> pops(size_t, bool = false);
		template <typename T> std::vector<T> pops(size_t, bool, bool);
		std::string popstr(bool = false);
		std::string popstr(size_t, bool = false);
		void SetEndian(bool);
		void Next(int);
		void Prev(int);
		void Reset();
		void Clear();
		size_t Size() const;
		size_t Remain();
		const byte* data();
		template<typename T> static MyBytes Create(T);
		MyBytes split(long long int = -1, long long int = -1) const;
};

template <typename T>
void MyBytes::push(T data)
{
	push<T>(data, defaultEndian);
}

template <typename T>
void MyBytes::push(T data, bool endian)
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
void MyBytes::push_head(T data)
{
	push_head<T>(data, defaultEndian);
}

template <typename T>
void MyBytes::push_head(T data, bool endian)
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
std::vector<T> MyBytes::pops(bool peek)
{
	return pops<T>(Remain(), peek, defaultEndian);
}

template <typename T>
std::vector<T> MyBytes::pops(bool peek, bool endian)
{
	return pops<T>(Remain(), peek, endian);
}

template <typename T>
std::vector<T> MyBytes::pops(size_t size, bool peek)
{
	return pops<T>(size, peek, defaultEndian);
}

template <typename T>
std::vector<T> MyBytes::pops(size_t size, bool peek, bool endian)
{
	if(size <= 0)
		size = Remain() - ptr;

	size_t cell = sizeof(T);
	if(ptr + cell * size > Remain())
		throw std::out_of_range("MyBytes : Byte Out of Range");

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
T MyBytes::pop(bool peek)
{
	return pop<T>(peek, defaultEndian);
}

template <typename T>
T MyBytes::pop(bool peek, bool endian)
{
	unsigned int len = sizeof(T) / sizeof(byte);
	if(ptr + len > bytes.size())
		throw std::out_of_range("MyBytes : Byte Out of Range");

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
MyBytes MyBytes::Create(T data)
{
	MyBytes result;
	result.push<T>(data);

	return result;
}