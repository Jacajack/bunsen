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

template <typename T>
class weak_resource_handle;

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
	auto get_name() const
	{
		std::scoped_lock lock{m_mutex};
		return m_name;
	}

	void set_name(const std::string &new_name)
	{
		m_manager->set_name(resource_id(), new_name);
	}

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

	//! Returns a size estimate
	virtual size_t estimate_resource_size() const
	{
		return sizeof(T);
	}

private:
	void set_manager(std::string name, bu::resource_manager<T> *mgr)
	{
		std::scoped_lock lock{m_mutex};
		m_name = std::move(name);
		m_manager = mgr;
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
	bu::resource_manager<T> *m_manager = nullptr;
};

template <typename T>
class resource_handle
{
	friend class resource<T>;
	friend class weak_resource_handle<T>;

	static_assert(std::is_base_of_v<bu::resource<T>, T>, "resource_handle bad type");

public:
	resource_handle() = delete;

	resource_handle(const resource_handle &src) :
		m_resource(src.m_resource)
	{
		m_resource->register_handle();
	}

	resource_handle(resource_handle &&src) :
		m_resource(src.m_resource)
	{
		m_resource->register_handle();
	}

	resource_handle(const weak_resource_handle<T> &src) :
		m_resource(src.m_resource)
	{
		m_resource->register_handle();
	}

	resource_handle &operator=(const resource_handle &rhs)
	{
		if (this == &rhs) return *this;
		m_resource->unregister_handle();
		m_resource = rhs.m_resource;
		m_resource->register_handle();
		return *this;
	}

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
class weak_resource_handle
{
	friend class resource_handle<T>;

public:
	weak_resource_handle(const resource_handle<T> &src) :
		m_resource(src.m_resource)
	{
	}

	weak_resource_handle() = default;
	weak_resource_handle(const weak_resource_handle &) = default;
	weak_resource_handle(weak_resource_handle &&) noexcept = default;
	weak_resource_handle &operator=(const weak_resource_handle &) = default;
	weak_resource_handle &operator=(weak_resource_handle &&) noexcept = default;
	~weak_resource_handle() = default;

	resource_handle<T> lock()
	{
		return resource_handle<T>{*this};
	}

	std::optional<resource_handle<T>> try_lock()
	{
		try
		{
			return resource_handle<T>{*this};
		}
		catch (const std::bad_weak_ptr &ex)
		{
			return {};
		}
	}

private:
	std::weak_ptr<T> m_resource;
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