#pragma once
#include <set>

namespace bu::rt {

class minmax_tracker
{
public:
	inline void add(float x);
	inline void remove(float x);
	inline float get_min() const;
	inline float get_max() const;

private:
	std::multiset<float> set;
};

void minmax_tracker::add(float x)
{
	set.insert(x);
}

void minmax_tracker::remove(float x)
{
	auto it = set.find(x);
	set.erase(it);
}

float minmax_tracker::get_min() const
{
	return *set.begin();
}

float minmax_tracker::get_max() const
{
	return *set.rbegin();
}

}