#pragma once
#include <mutex>
#include <set>
#include <queue>
#include <cmath>
#include <memory>
#include "MyTypes.hpp"

/**
 * @brief queue element for Match Maker.
 * 
 */
class queue_element
{
private:
	Account_ID_t id;
	int win;
	int draw;
	int loose;
	const std::shared_ptr<int> wait_count;

	bool isMatchable(const queue_element&) const;
	void Wait() const;

	static double Interpolate;
	
public:
	/**
	 * @brief Constructor of empty queue element.
	 * 
	 */
	queue_element();
	/**
	 * @brief Constructor of queue element.
	 * 
	 * @param _id player's id
	 * @param _win win count of the player.
	 * @param _draw draw count of the player.
	 * @param _loose loose count of the player.
	 */
	queue_element(Account_ID_t _id, int _win, int _draw, int _loose);
	bool operator==(const queue_element&) const;
	bool operator<(const queue_element&) const;
	friend void swap(queue_element&, queue_element&);
	Account_ID_t whoami() const;
	double ratio() const;

	friend class MyMatchMaker;
};

/**
 * @brief Match Maker
 * 
 */
class MyMatchMaker
{
private:
	std::mutex mtx;
	std::set<queue_element> wait_queue;
	std::queue<std::pair<queue_element, queue_element>> match_queue;
	using ulock = std::unique_lock<std::mutex>;

public:
	/**
	 * @brief Constructor of Match Maker.
	 * 
	 * @param interpolate_ratio compensating value for waiting. Matching Coverage will be increase by this value and waiting delay.
	 */
	MyMatchMaker(const double &interpolate_ratio);
	~MyMatchMaker() = default;

	/**
	 * @brief Enqueue player into Matching Queue.
	 * 
	 * @param id player's id
	 * @param win win count of the player
	 * @param draw draw count of the player
	 * @param loose loose count of the player
	 */
	void Enter(Account_ID_t id, int win, int draw, int loose);
	/**
	 * @brief Reenqueue player into Matching Queue.
	 * 
	 * @param elem latest information of the player.
	 */
	void Enter(queue_element elem);
	/**
	 * @brief Dequeue player from Matching Queue.
	 * 
	 * @param id player's id
	 */
	void Exit(Account_ID_t id);

	/**
	 * @brief Make Matches according to player-waiting queue.
	 * 
	 */
	void Process();
	/**
	 * @brief is there any match created by 'Process()'?
	 * 
	 * @return true 
	 * @return false 
	 */
	bool isThereMatch();
	/**
	 * @brief Get a created match. If there is no match, return value will be empty queue_element.
	 * 
	 * @return std::pair<queue_element, queue_element> 
	 */
	std::pair<queue_element, queue_element> GetMatch();
	/**
	 * @brief remove lately-created match
	 * 
	 */
	void PopMatch();

	/**
	 * @brief Get the number of players who waits match.
	 * 
	 * @return size_t number of players.
	 */
	size_t Size() const;
};