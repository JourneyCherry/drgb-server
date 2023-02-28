#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ByteQueue.hpp"
#include "StackTraceExcept.hpp"
#include "MyTypes.hpp"

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
		std::queue<ByteQueue> msgs;
		std::vector<byte> recvbuffer;
		std::mutex mtx;
		std::condition_variable cv;
		void flush();
		int split();
		ByteQueue decapsulate(std::vector<byte>);
		ByteQueue decrypt(std::vector<byte>);
		ByteQueue depackage(std::vector<byte>);
		static std::vector<byte> encapsulate(ByteQueue);
		static std::vector<byte> encrypt(ByteQueue);
		
	public:
		PacketProcessor();
		~PacketProcessor();
		void Start();	//JoinMsg()가 Message를 못받고 
		void Stop();	//
		void Recv(const byte*, int);
		void Recv(const ByteQueue&);
		bool isMsgIn();
		ByteQueue GetMsg();
		ByteQueue JoinMsg();
		void Clear();
		static std::vector<byte> enpackage(ByteQueue);
};

}
}