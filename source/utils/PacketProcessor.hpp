#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "Expected.hpp"
#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

class PacketProcessor
{
	public:
		using locker = std::unique_lock<std::mutex>;
	private:
		const static byte eof = 0xff;
		const static byte pattern = 0xee;
		
		bool isRunning;
		std::queue<std::vector<byte>> msgs;
		std::vector<byte> recvbuffer;
		std::mutex mtx;
		std::condition_variable cv;
		void flush();
		int split();
		
	public:
		PacketProcessor();
		~PacketProcessor();
		void Start();
		void Stop();
		void Recv(const byte*, int);
		bool isMsgIn();
		std::vector<byte> GetMsg();
		Expected<std::vector<byte>> JoinMsg();
		void Clear();
		static std::vector<byte> decapsulate(std::vector<byte>);
		static std::vector<byte> encapsulate(std::vector<byte>);
};

}
}