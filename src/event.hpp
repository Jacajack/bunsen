#pragma once
#include <list>
#include <queue>
#include <memory>
#include <functional>

namespace bu {

enum class event_type
{
	EMPTY,
	SCENE_TRANSFORM_STARTED,
	SCENE_TRANSFORM_ABORTED,
	SCENE_TRANSFORM_FINISHED,
	SCENE_MODIFIED,
	MATERIAL_MODIFIED,
};

class event
{
public:
	event() = default;
	event(event_type t) :
		m_type(t)
	{}

	event_type get_type() const {return m_type;}

private:
	event_type m_type = event_type::EMPTY;
};

class event_bus;

/**
	\brief Allows to listen to events on event_bus
*/
class event_bus_connection : public std::enable_shared_from_this<event_bus_connection>
{
	friend class event_bus;

public:
	event_bus_connection(const event_bus_connection &) = delete;
	event_bus_connection(event_bus_connection &&) noexcept = default;
	event_bus_connection &operator=(const event_bus_connection &) = delete;
	event_bus_connection &operator=(event_bus_connection &&) noexcept = default;

	void clear();
	void emit(const event &ev);
	bool poll(event &ev);
	void process_events(std::function<void(const event &ev)> handler);
	std::shared_ptr<bu::event_bus> get_bus() const;

protected:
	event_bus_connection() = default;
	event_bus_connection(std::shared_ptr<event_bus> bus);

	void push_event(const event &ev);

	std::shared_ptr<event_bus> m_bus;
	std::queue<event> m_queue;
};

/**
	\brief Transmits events
*/
class event_bus : public std::enable_shared_from_this<event_bus>
{
	friend class event_bus_connection;

public:
	std::shared_ptr<event_bus_connection> make_connection();
	void direct_emit(const event &ev);

protected:
	void emit(const event &ev, std::shared_ptr<event_bus_connection> skip = {});

	std::list<std::weak_ptr<event_bus_connection>> m_connections;
};

using event_bus_ptr = std::shared_ptr<event_bus>;
using event_bus_connection_ptr = std::shared_ptr<event_bus_connection>;

}