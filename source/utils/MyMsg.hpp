#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include "MyBytes.hpp"
#include "MyExcepts.hpp"
#include "MyTypes.hpp"

class MyMsg
{
	public:
		using locker = std::unique_lock<std::mutex>;
	private:
		const static byte eof = 0xff;
		const static byte pattern = 0xee;
		
		bool isRunning;
		std::queue<MyBytes> msgs;
		std::vector<byte> recvbuffer;
		std::mutex mtx;
		std::condition_variable cv;
		void flush();
		int split();
		MyBytes decapsulate(std::vector<byte>);
		MyBytes decrypt(std::vector<byte>);
		MyBytes depackage(std::vector<byte>);
		static std::vector<byte> encapsulate(MyBytes);
		static std::vector<byte> encrypt(MyBytes);
		
	public:
		MyMsg();
		~MyMsg();
		void Start();	//JoinMsg()가 Message를 못받고 
		void Stop();	//
		void Recv(const byte*, int);
		void Recv(const MyBytes&);
		bool isMsgIn();
		MyBytes GetMsg();
		MyBytes JoinMsg();
		void Clear();
		static std::vector<byte> enpackage(MyBytes);
};
