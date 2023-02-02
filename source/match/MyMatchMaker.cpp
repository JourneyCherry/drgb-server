#include "MyMatchMaker.hpp"

queue_element::queue_element() : id(0), win(0), draw(0), loose(0), wait_count(std::make_shared<int>(1))
{

}

queue_element::queue_element(Account_ID_t _id, int _win, int _draw, int _loose) : wait_count(std::make_shared<int>(1))
{
	*wait_count = 1;
	id = _id;

	win = _win;
	draw = _draw;
	loose = _loose;
}

bool queue_element::operator==(const queue_element& rhs) const
{
	return (id == rhs.id);
}

bool queue_element::operator<(const queue_element& rhs) const
{
	if (ratio() == rhs.ratio())
		return (id < rhs.id);
	else
		return (ratio() < rhs.ratio());
}

bool queue_element::isMatchable(const queue_element& rhs) const
{
	double diff = std::abs(ratio() - rhs.ratio());
	double coverage = std::max((*wait_count) * Interpolate, (*rhs.wait_count) * Interpolate);
	return diff <= coverage;
}

void queue_element::Wait() const
{
	(*wait_count)++;
}

Account_ID_t queue_element::whoami() const
{
	return id;
}

double queue_element::ratio() const
{
	int sum = win + loose + draw;
	if (sum == 0)
	{
		sum = 1;
		*wait_count = 10;
	}
	return (double)win / (double)sum;
}

void MyMatchMaker::Enter(Account_ID_t id, int win, int draw, int loose)
{
	ulock lk(mtx);
	wait_queue.emplace(id, win, draw, loose);
}

void MyMatchMaker::Enter(queue_element elem)
{
	if(elem.whoami() <= 0)
		return;
	
	ulock lk(mtx);
	wait_queue.insert(elem);
}

void MyMatchMaker::Exit(Account_ID_t id)
{
	ulock lk(mtx);
	for (auto i = wait_queue.begin(); i != wait_queue.end(); i++)
	{
		if (i->whoami() == id)
		{
			wait_queue.erase(i);
			return;
		}
	}
}

void MyMatchMaker::Process()
{
	ulock lk(mtx);
	for (auto i = wait_queue.begin(); i != wait_queue.end();)
	{
		bool isMatched = false;
		for (auto j = std::next(i); j != wait_queue.end(); j++)
		{
			if (i->isMatchable(*j))
			{
				match_queue.push({ *i, *j });
				wait_queue.erase(j);
				i = wait_queue.erase(i);
				isMatched = true;
				break;
			}
		}
		if (!isMatched)
			i++;
	}
	for (auto i = wait_queue.begin();i!=wait_queue.end();i++)
	{
		i->Wait();
	}
}

bool MyMatchMaker::isThereMatch()
{
	return !match_queue.empty();
}

std::pair<queue_element, queue_element> MyMatchMaker::GetMatch()
{
	if (match_queue.empty())
		return { queue_element(), queue_element() };
	auto result = match_queue.front();

	return result;
}

void MyMatchMaker::PopMatch()
{
	if(match_queue.empty())
		return;
	match_queue.pop();
}