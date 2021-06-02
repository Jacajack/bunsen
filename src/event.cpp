#include "event.hpp"

using bu::event_bus;
using bu::event_bus_connection;

void event_bus_connection::clear()
{
	while (!m_queue.empty())
		m_queue.pop();
}

void event_bus_connection::emit(const event &ev)
{
	m_bus->emit(ev);
}

bool event_bus_connection::poll(event &ev)
{
	if (m_queue.empty()) return false;
	else
	{
		ev = m_queue.front();
		m_queue.pop();
		return true;
	}
}

void event_bus_connection::process_events(std::function<void(const event &ev)> handler)
{
	event ev;
	while (poll(ev))
		handler(ev);
}

event_bus_connection::event_bus_connection(std::shared_ptr<event_bus> bus) :
	m_bus(bus)
{
}

void event_bus_connection::push_event(const event &ev)
{
	m_queue.push(ev);
}

std::shared_ptr<bu::event_bus> event_bus_connection::get_bus() const
{
	return m_bus;
}

std::shared_ptr<event_bus_connection> event_bus::make_connection()
{
	event_bus_connection tmp(this->shared_from_this());
	auto ptr = std::make_shared<event_bus_connection>(std::move(tmp));
	m_connections.push_back(ptr);
	return ptr;
}

void event_bus::direct_emit(const event &ev)
{
	emit(ev);
}

void event_bus::emit(const event &ev, std::shared_ptr<event_bus_connection> skip)
{
	for (auto it = m_connections.begin(); it != m_connections.end();)
	{
		if (auto ptr = it->lock())
		{
			if (ptr != skip)
			{
				ptr->push_event(ev);
				++it;
			}
		}
		else
			it = m_connections.erase(it);
	}
}