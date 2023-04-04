#include "ByteQueue.hpp"

namespace mylib{
namespace utils{

bool ByteQueue::g_isLittleEndian = ByteQueue::isLittleEndianSystem();

bool ByteQueue::isLittleEndianSystem()
{
	int num = 1;
	return (((unsigned char*)(&num))[0] == 1);
}

ByteQueue::ByteQueue()
{
	defaultEndian = g_isLittleEndian;
	ptr = 0;
}

ByteQueue::ByteQueue(std::vector<byte> bstream)
{
	int len = bstream.size();
	for(int i = 0;i<len;i++)
		bytes.push_back(bstream[i]);
	defaultEndian = g_isLittleEndian;
	ptr = 0;
}

ByteQueue::ByteQueue(bool endian)
{
	defaultEndian = endian;
	ptr = 0;
}

ByteQueue::ByteQueue(const char *buf, size_t size)
{
	defaultEndian = g_isLittleEndian;
	for(int i = 0;i<size;i++)
		bytes.push_back(buf[i]);
	ptr = 0;
}

ByteQueue::ByteQueue(const ByteQueue& copy)
{
	defaultEndian = copy.defaultEndian;
	ptr = copy.ptr;
	bytes = copy.bytes;
}

ByteQueue::ByteQueue(ByteQueue&& move) noexcept
{
	defaultEndian = move.defaultEndian;
	ptr = move.ptr;
	bytes = move.bytes;

	move.ptr = 0;
	move.bytes.clear();
}

ByteQueue &ByteQueue::operator=(const ByteQueue& copy)
{
	defaultEndian = copy.defaultEndian;
	ptr = copy.ptr;
	bytes = copy.bytes;

	return *this;
}

ByteQueue& ByteQueue::operator=(ByteQueue&& move)
{
	defaultEndian = move.defaultEndian;
	ptr = move.ptr;
	bytes = move.bytes;

	move.ptr = 0;
	move.bytes.clear();

	return *this;
}

ByteQueue ByteQueue::operator+(const ByteQueue& adder) const
{
	ByteQueue result(*this);
	result.bytes.insert(result.bytes.end(), adder.bytes.begin(), adder.bytes.end());

	return result;
}

byte ByteQueue::operator[](long unsigned int index) const
{
	if(index < 0 || index >= bytes.size())
		throw std::out_of_range("ByteQueue::operator[long unsigned int] : Byte Out of Range");
	return bytes[index];
}

ByteQueue& ByteQueue::operator+=(const ByteQueue& adder)
{
	this->bytes.insert(this->bytes.end(), adder.bytes.begin(), adder.bytes.end());

	return *this;
}

ByteQueue::~ByteQueue()
{
	ptr = 0;
	bytes.clear();
}

void ByteQueue::push(const byte* datas, int len)
{
	for(int i = 0;i<len;i++)
		bytes.push_back(datas[i]);
}

std::string ByteQueue::popstr(bool peek)
{
	return popstr(Remain(), peek);
}

std::string ByteQueue::popstr(size_t len, bool peek)
{
	if(ptr + len > bytes.size())
		throw std::out_of_range("ByteQueue::pop(int) : Byte Out of Range");

	std::vector<byte> bstream;

	for(int i = 0;i<len;i++)
		bstream.push_back(bytes[i + ptr]);

	std::string result((const char*)bstream.data(), bstream.size());	//TODO : UTF-8 encoded Byte Array를 string으로 변환과정 필요.
	
	if(!peek)
		ptr += len;

	return result;
}

void ByteQueue::SetEndian(bool endian)
{
	defaultEndian = endian;
}

void ByteQueue::Next(int len)
{
	if(ptr + len > bytes.size())
		throw std::out_of_range("ByteQueue::Next(int) : Byte Out of Range");

	ptr += len;
}

void ByteQueue::Prev(int len)
{
	if(len < 0)
		ptr = 0;
	else
		ptr = std::max(ptr - len, (long unsigned int)0);
}

void ByteQueue::Reset()
{
	ptr = 0;
}

void ByteQueue::Clear()
{
	Reset();
	bytes.clear();
}

size_t ByteQueue::Size() const
{
	return bytes.size();
}

size_t ByteQueue::Remain()
{
	return bytes.size() - ptr;
}

const byte* ByteQueue::data()
{
	return bytes.data();
}

const std::vector<byte> ByteQueue::vector()
{
	return bytes;
}

ByteQueue ByteQueue::split(long long int start, long long int end) const
{
	ByteQueue result;
	if(start < 0 || start >= bytes.size())
		start = ptr;
	if(end < 0 || end >= bytes.size())
		end = bytes.size();
	result.bytes.insert(result.bytes.begin(), bytes.begin() + start, bytes.begin() + end);
	result.defaultEndian = defaultEndian;
	result.ptr = ptr - start;

	return result;
}

}
}