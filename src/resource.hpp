#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <memory>
#include "mrsw.hpp"
#include "utils.hpp"
#include "uid_provider.hpp"

namespace bu {

template <typename T>
class resource_manager;

template <typename T>
class resource_handle;


/**
	Resources should inherit from this class
*/
template <typename T>
class resource : public std::enable_shared_from_this<T>, private bu::uid_provider<resource<T>>
{
	friend class resource_handle<T>;

public:
	resource() = default;
	auto resource_id() const {return uid_provider<resource<T>>::uid();}
	auto handle_count() const {return m_handles;}
	auto is_expired() const {return m_expired;}

	bool operator==(const resource<T> &rhs) const
	{
		return resource_id() == rhs.resource_id();
	}

	resource_handle<T> get_handle()
	{
		return resource_handle<T>{static_cast<T&>(*this)};
	}

	resource_handle<const T> get_handle() const
	{
		return resource_handle<const T>{static_cast<const T&>(*this)};
	}

	void expire()
	{
		std::scoped_lock lock{m_mutex};
		if (m_handles != 0)
			throw std::runtime_error{"expire() on resource with active handles"};
		m_expired = true;
	}

private:
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
};

template <typename T>
class resource_handle
{
	friend class resource<T>;

	static_assert(std::is_base_of_v<bu::resource<T>, T>, "resource_handle bad type");

public:
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

/**
	This class actually contains the resource data.
*/
template <typename T>
class mrsw_resource : public resource<mrsw_resource<T>>, public mrsw<T>
{
public:
	using mrsw<T>::mrsw;
};


}