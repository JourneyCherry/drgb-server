#include "MyBytes.hpp"

bool MyBytes::g_isLittleEndian = MyBytes::isLittleEndianSystem();

bool MyBytes::isLittleEndianSystem()
{
	int num = 1;
	return (((unsigned char*)(&num))[0] == 1);
}

MyBytes::MyBytes()
{
	defaultEndian = g_isLittleEndian;
	ptr = 0;
}

MyBytes::MyBytes(std::vector<byte> bstream)
{
	int len = bstream.size();
	for(int i = 0;i<len;i++)
		bytes.push_back(bstream[i]);
	defaultEndian = g_isLittleEndian;
	ptr = 0;
}

MyBytes::MyBytes(bool endian)
{
	defaultEndian = endian;
	ptr = 0;
}

MyBytes::MyBytes(const char *buf, size_t size)
{
	defaultEndian = g_isLittleEndian;
	for(int i = 0;i<size;i++)
		bytes.push_back(buf[i]);
	ptr = 0;
}

MyBytes::MyBytes(const MyBytes& copy)
{
	defaultEndian = copy.defaultEndian;
	ptr = copy.ptr;
	bytes = copy.bytes;
}

MyBytes::MyBytes(MyBytes&& move) noexcept
{
	defaultEndian = move.defaultEndian;
	ptr = move.ptr;
	bytes = move.bytes;

	move.ptr = 0;
	move.bytes.clear();
}

MyBytes &MyBytes::operator=(const MyBytes& copy)
{
	defaultEndian = copy.defaultEndian;
	ptr = copy.ptr;
	bytes = copy.bytes;

	return *this;
}

MyBytes& MyBytes::operator=(MyBytes&& move)
{
	defaultEndian = move.defaultEndian;
	ptr = move.ptr;
	bytes = move.bytes;

	move.ptr = 0;
	move.bytes.clear();

	return *this;
}

MyBytes MyBytes::operator+(const MyBytes& adder) const
{
	MyBytes result(*this);
	result.bytes.insert(result.bytes.end(), adder.bytes.begin(), adder.bytes.end());

	return result;
}

byte MyBytes::operator[](long unsigned int index) const
{
	if(index < 0 || index >= bytes.size())
		throw std::out_of_range("MyBytes::operator[long unsigned int] : Byte Out of Range");
	return bytes[index];
}

MyBytes& MyBytes::operator+=(const MyBytes& adder)
{
	this->bytes.insert(this->bytes.end(), adder.bytes.begin(), adder.bytes.end());

	return *this;
}

MyBytes::~MyBytes()
{
	ptr = 0;
	bytes.clear();
}

void MyBytes::push(const byte* datas, int len)
{
	for(int i = 0;i<len;i++)
		bytes.push_back(datas[i]);
}

std::string MyBytes::popstr(bool peek)
{
	return popstr(Remain(), peek);
}

std::string MyBytes::popstr(size_t len, bool peek)
{
	if(ptr + len > bytes.size())
		throw std::out_of_range("MyBytes::pop(int) : Byte Out of Range");

	std::vector<byte> bstream;

	for(int i = 0;i<len;i++)
		bstream.push_back(bytes[i + ptr]);

	std::string result((const char*)bstream.data(), bstream.size());	//TODO : UTF-8 encoded Byte Array를 string으로 변환과정 필요.
	
	if(!peek)
		ptr += len;

	return result;
}

void MyBytes::SetEndian(bool endian)
{
	defaultEndian = endian;
}

void MyBytes::Next(int len)
{
	if(ptr + len > bytes.size())
		throw std::out_of_range("MyBytes::Next(int) : Byte Out of Range");

	ptr += len;
}

void MyBytes::Prev(int len)
{
	if(len < 0)
		ptr = 0;
	else
		ptr = std::max(ptr - len, (long unsigned int)0);
}

void MyBytes::Reset()
{
	ptr = 0;
}

void MyBytes::Clear()
{
	Reset();
	bytes.clear();
}

size_t MyBytes::Size() const
{
	return bytes.size();
}

size_t MyBytes::Remain()
{
	return bytes.size() - ptr;
}

const byte* MyBytes::data()
{
	return bytes.data();
}

MyBytes MyBytes::split(long long int start, long long int end) const
{
	MyBytes result;
	if(start < 0 || start >= bytes.size())
		start = ptr;
	if(end < 0 || end >= bytes.size())
		end = bytes.size();
	result.bytes.insert(result.bytes.begin(), bytes.begin() + start, bytes.begin() + end);
	result.defaultEndian = defaultEndian;
	result.ptr = ptr - start;

	return result;
}