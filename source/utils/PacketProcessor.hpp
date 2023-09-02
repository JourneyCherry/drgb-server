#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "Expected.hpp"
#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

/**
 * @brief Data Stream Interpreter. It keeps messages from fragmentization.
 * 
 */
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
		/**
		 * @brief Restart Processor
		 * 
		 */
		void Start();
		/**
		 * @brief Stop Processor. All thread waiting message will be waken up.
		 * 
		 */
		void Stop();
		/**
		 * @brief Receive bytes from stream.
		 * 
		 * @param data byte data pointer
		 * @param len byte data size to read
		 */
		void Recv(const byte *data, int len);
		/**
		 * @brief Check complete message is in.
		 * 
		 * @return true if there are more than one complete message.
		 * @return false if there is no complete message.
		 */
		bool isMsgIn();
		/**
		 * @brief Get one latest complete message. Message Queue should be checked by using 'isMsgIn()' before Getting Message. If you want to synchronously receive complete messages, use 'JoinMsg()'.
		 * 
		 * @return std::vector<byte> byte stream message.
		 * @throw StackTraceExcept when there is no complete message.
		 */
		std::vector<byte> GetMsg();
		/**
		 * @brief Wait until a complete message is received for 'dur' ms. If Processor is stopped, it returns without any messages. If you want to asynchronously receive complete messages, use 'isMsgIn()' and 'GetMsg()'.
		 * 
		 * @param dur millisecond time for wait. if it is 0 ms, it waits forever.
		 * @return Expected<std::vector<byte>, ErrorCode> result of wait.
		 * @return std::vector<byte> a complete byte message
		 * @return ErrorCode(ERR_TIMEOUT) there is no message in 'dur' ms.
		 * @return ErrorCode(ERR_CONNECTION_CLOSED) Processor is stopped by 'Stop()'.
		 */
		Expected<std::vector<byte>, ErrorCode> JoinMsg(std::chrono::milliseconds dur);
		/**
		 * @brief Clear all messages received including incomplete messages.
		 * 
		 */
		void Clear();
		/**
		 * @brief Decapsulate bytes and Get plain message.
		 * 
		 * @param data Capsulated bytes.
		 * @return std::vector<byte> plain message
		 */
		static std::vector<byte> decapsulate(std::vector<byte> data);
		/**
		 * @brief Encapsulate a message and Get capsulated bytes
		 * 
		 * @param data plain message
		 * @return std::vector<byte> capsulated bytes
		 */
		static std::vector<byte> encapsulate(std::vector<byte> data);
};

}
}