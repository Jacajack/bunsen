#pragma once
#include <atomic>
#include <mutex>
#include <memory>
#include "mrsw.hpp"
#include "utils.hpp"
#include "uid_provider.hpp"
#include "log.hpp"

namespace bu {

template <typename T>
class resource_manager;

template <typename T>
class resource_handle;

/**
	Resources should inherit from this class

	\todo mutex on the name
	\todo locks in is_expired() and handle_count()
*/
template <typename T>
class resource : public std::enable_shared_from_this<T>, private bu::uid_provider<resource<T>>
{
	friend class resource_handle<T>;
	friend class resource_manager<T>;

public:
	resource() = default;
	virtual ~resource()
	{
		LOG_DEBUG << "freeing resource '" << get_name() << "' (id=" << resource_id() << ")";
	}

	auto resource_id() const {return uid_provider<resource<T>>::uid();}
	auto get_name() const {return m_name;}
	auto handle_count() const {return m_handles;}
	auto is_expired() const {return m_expired;}
	auto can_delete() const
	{
		std::scoped_lock lock{m_mutex};
		return m_expired && m_handles == 0;
	}

	//! Compares resources
	bool operator==(const resource<T> &rhs) const
	{
		return resource_id() == rhs.resource_id();
	}

	//! Returns a handle
	resource_handle<T> get_handle()
	{
		return resource_handle<T>{static_cast<T&>(*this)};
	}

	//! Returns a const handle
	resource_handle<const T> get_handle() const
	{
		return resource_handle<const T>{static_cast<const T&>(*this)};
	}

	//! Attempts to expire the resource. Throws if in use
	void try_expire()
	{
		std::scoped_lock lock{m_mutex};
		if (m_handles != 0)
			throw std::runtime_error{"expire() on resource with active handles"};
		m_expired = true;
	}

	//! Forcefully expires the resource
	void force_expire()
	{
		std::scoped_lock lock{m_mutex};
		m_expired = true;
	}

private:
	void set_name(std::string name)
	{
		m_name = std::move(name);
	}

	void register_handle() const
	{
		std::scoped_lock lock{m_mutex};
		if (m_expired)
			throw std::runtime_error{"register_handle() on expired resource"};
		m_handles++;
	}

	void unregister_handle() const
	{
		std::scoped_lock lock{m_mutex};
		if (m_expired)
			throw std::runtime_error{"unregister_handle() on expired resource"};
		if (m_handles == 0)
			throw std::runtime_error{"unregister_handle() on resource with no handles"};
		m_handles--;
	}

	mutable std::mutex m_mutex;
	mutable int m_handles = 0;
	bool m_expired = false;
	std::string m_name;
};

template <typename T>
class resource_handle : public bu::no_copy
{
	friend class resource<T>;

	static_assert(std::is_base_of_v<bu::resource<T>, T>, "resource_handle bad type");

public:
	resource_handle() = delete;
	resource_handle(resource_handle &&src) :
		m_resource(src.m_resource)
	{
		m_resource->register_handle();
	}

	resource_handle &operator=(const resource_handle &rhs) = delete;
	resource_handle &operator=(resource_handle &&rhs)
	{
		if (this == &rhs) return *this;
		m_resource = rhs.m_resource;
		m_resource->register_handle();
		return *this;
	}

	~resource_handle()
	{
		m_resource->unregister_handle();
	}

	const T *operator->() const {return m_resource.get();}
	T *operator->() {return m_resource.get();}
	const T &operator*() const {return *m_resource;}
	T &operator*() {return *m_resource;}

private:
	explicit resource_handle(T &res) : 
		m_resource(res.shared_from_this())
	{
		m_resource->register_handle();
	}

	std::shared_ptr<T> m_resource;
};

template <typename T>
struct simple_resource : public bu::resource<simple_resource<T>>, public T
{
	using T::T;
};

template <typename T>
struct mrsw_resource : public bu::resource<mrsw_resource<T>>, public mrsw<T>
{
	using mrsw<T>::mrsw;
};

}