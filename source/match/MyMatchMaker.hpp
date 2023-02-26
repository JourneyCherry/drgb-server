#pragma once
#include <mutex>
#include <set>
#include <queue>
#include <cmath>
#include <memory>

typedef unsigned char byte;
typedef int32_t Int;
typedef uint32_t UInt;
typedef unsigned long long Account_ID_t;

class queue_element
{
private:
	static constexpr double Interpolate = 0.1f;	//TODO : 이것을 Config로 처리하자.
	Account_ID_t id;
	int win;
	int draw;
	int loose;
	const std::shared_ptr<int> wait_count;

public:
	queue_element();
	queue_element(Account_ID_t, int, int, int);
	bool operator==(const queue_element&) const;
	bool operator<(const queue_element&) const;
	bool isMatchable(const queue_element&) const;
	void Wait() const;
	Account_ID_t whoami() const;
	double ratio() const;
};

class MyMatchMaker
{
private:
	std::mutex mtx;
	std::set<queue_element> wait_queue;
	std::queue<std::pair<queue_element, queue_element>> match_queue;
	using ulock = std::unique_lock<std::mutex>;

public:
	MyMatchMaker() = default;
	~MyMatchMaker() = default;

	void Enter(Account_ID_t, int, int, int);
	void Enter(queue_element);
	void Exit(Account_ID_t);

	void Process();
	bool isThereMatch();
	std::pair<queue_element, queue_element> GetMatch();
	void PopMatch();
};