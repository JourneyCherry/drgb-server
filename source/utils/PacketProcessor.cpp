#include "PacketProcessor.hpp"

namespace mylib{
namespace utils{

// PacketProcessor::encapsulate()의 std::vector<byte>::push_back()에 const static을 넣기 위해선 Definition이 필요한데, .hpp파일에 선언된 것은 Declaration이므로, 여기서의 Definition이 필요하다.
const byte PacketProcessor::pattern;
const byte PacketProcessor::eof;

PacketProcessor::PacketProcessor()
{
	isRunning = true;
}

PacketProcessor::~PacketProcessor()
{
	isRunning = false;
	cv.notify_all();
	//소멸자에서 mutex.lock()을 수행하는 것은 좋지 못한 방법이다.
	while (msgs.size() > 0)
		msgs.pop();
	recvbuffer.clear();
}

void PacketProcessor::Start()
{
	isRunning = true;
}

void PacketProcessor::Stop()
{
	isRunning = false;
	cv.notify_all();
	Clear();
}

void PacketProcessor::flush()
{
	int ptr = split();
	while (ptr >= 0)	//빈 메시지가 Encapsulate되어 오면 ptr이 0이 된다.
	{
		std::vector<byte> data(recvbuffer.begin(), recvbuffer.begin() + ptr);
		recvbuffer.erase(recvbuffer.begin(), recvbuffer.begin() + ptr + 2);
		if(data.size() > 0)
			msgs.push(decapsulate(data));
		ptr = split();
	}
}

int PacketProcessor::split()
{
	int size = recvbuffer.size();
	for (int i = 0; i < size - 1; i++)
	{
		if(recvbuffer[i] == pattern && recvbuffer[i+1] == pattern)
		{
			i++;
			continue;
		}
		if (recvbuffer[i] == pattern && recvbuffer[i + 1] == eof)
			return i;
	}
	return -1;
}

std::vector<byte> PacketProcessor::decapsulate(std::vector<byte> data)
{
	std::vector<byte> result;
	result.reserve(data.size());
	for (auto i = data.begin(); i != data.end(); i++)
	{
		if (*i == pattern)
		{
			i++;
			if(*i == eof)
				return result;
		}
		result.push_back(*i);
	}
	return result;
}

std::vector<byte> PacketProcessor::encapsulate(std::vector<byte> data)
{
	std::vector<byte> result;
	result.reserve(data.size() * 2 + 2);	//Maximum Possible Size
	for(const byte &b : data)
	{
		if (b == pattern)
			result.push_back(pattern);
		result.push_back(b);
	}
	result.push_back(pattern);
	result.push_back(eof);

	return result;
}

void PacketProcessor::Recv(const byte *data, int len)
{
	locker lk(mtx);
	for (int i = 0; i < len; i++)
		recvbuffer.push_back(data[i]);
	flush();
	if(msgs.size() > 0)
		cv.notify_one();	//JoinMsg()는 항상 1개의 thread 에서만 대기해야 한다.
}

bool PacketProcessor::isMsgIn()	//TODO : 보통 GetMsg를 호출하기 전에 필수적으로 isMsgIn을 호출하지만, 두 메소드 사이에서 타 스레드의 접근이 있을 수 있으므로, 둘을 합칠 필요가 있을 수 있다. 즉, 분리된 상태는 100% Thread-safe하다고 할 수 없다.
{
	return msgs.size() > 0;
}

std::vector<byte> PacketProcessor::GetMsg()
{
	locker lk(mtx);
	if (msgs.empty())
		throw StackTraceExcept("PacketProcessor is Empty", __STACKINFO__);
	auto result = msgs.front();
	msgs.pop();

	return result;
}

Expected<std::vector<byte>> PacketProcessor::JoinMsg()
{
	locker lk(mtx);
	cv.wait(lk, [&](){return isMsgIn() || !isRunning;});
	if(!isRunning)
		return {};
	auto result = msgs.front();
	msgs.pop();
	lk.unlock();

	return result;
}

void PacketProcessor::Clear()
{
	locker lk(mtx);
	while (msgs.size() > 0)
		msgs.pop();
	recvbuffer.clear();
}

}
}