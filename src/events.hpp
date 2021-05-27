#pragma once
#include <list>
#include <queue>
#include <memory>

namespace bu {

enum class bunsen_event_type
{
	SCENE_TRANSFORM_STARTED,
	SCENE_TRANSFORM_ABORTED,
	SCENE_TRANSFORM_FINISHED,
	SCENE_MODIFIED
};

struct bunsen_event
{
	bunsen_event_type type;
};

template <typename T>
class event_listener;

template <typename T = bunsen_event>
class event_source
{
public:
	void emit(const T &ev)
	{
		for (auto it = m_listeners.begin(); it != m_listeners.end();)
		{
			if (auto ptr = it->lock())
			{
				ptr->push_event(ev);
				++it;
			}
			else
				it = m_listeners.erase(it);
		}
	}

	void add_listener(std::shared_ptr<event_listener<T>> listener)
	{
		m_listeners.emplace_back(listener);
	}

	void remove_listener(std::shared_ptr<event_listener<T>> listener)
	{
		m_listeners.remove(listener);
	}

protected:
	std::list<std::weak_ptr<event_listener<T>>> m_listeners;
};

template <typename T = bunsen_event>
class event_listener : public std::enable_shared_from_this<event_listener<T>>
{
	friend class event_source<T>;

public:
	event_listener() = default;
	
	void add_source(event_source<T> &src)
	{
		src->add_listener(this->shared_from_this());
	}

	void remove_source(event_source<T> &src)
	{
		src->remove_listener(this->shared_from_this());
	}

	bool poll(T &ev)
	{
		if (m_queue.empty()) return false;
		else
		{
			ev = m_queue.front();
			m_queue.pop();
			return true;
		}
	}

	void clear()
	{
		while (!m_queue.empty())
			m_queue.pop();
	}

protected:
	void push_event(const T &ev)
	{
		m_queue.push(ev);
	}

	std::queue<T> m_queue;
};

using bunsen_event_source = event_source<bunsen_event>;
using bunsen_event_listener = event_listener<bunsen_event>;

}