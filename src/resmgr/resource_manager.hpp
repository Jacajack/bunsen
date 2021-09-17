#pragma once
#include <type_traits>
#include <memory>
#include <map>
#include <atomic>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include "resource.hpp"
#include "name_manager.hpp"
#include "log.hpp"

namespace bu {

/**
	\todo THREAD SAFETY
*/
template <typename T>
class resource_manager
{
	static_assert(std::is_base_of_v<bu::resource<T>, T>,
		"Resource manager can only manage classes derived from bu::managed_resource<T>");

public:
	using id_type = typename bu::uid_provider<T>::id_type;

	//! Adds a resource to the manager
	resource_handle<T> add(const std::string &name, std::unique_ptr<T> ptr)
	{
		// Associate the ID with a shared pointer
		auto id = ptr->resource_id();
		std::shared_ptr<T> shared{std::move(ptr)};
		auto [it, insert_ok] = m_resources.insert({id, shared});
		auto &res = *it->second;

		if (!insert_ok)
			throw std::runtime_error{"resource_manager add() failed to insert"};

		// Register name (or suggest a new one)
		std::string new_name = m_name_manager.add(name);
		m_ids[new_name] = id;
		res.set_name(new_name);

		LOG_INFO << "new resource '" << new_name << "' (id=" << id << ")";

		return shared->get_handle();
	}

	//! Constructs a resource
	template <typename... Args>
	resource_handle<T> emplace(const std::string &name, Args... args)
	{
		return add(name, std::make_unique<T>(args...));
	}

	//! Marks all unused resources as expired
	void expire_unused()
	{
		for (auto &[id, res] : m_resources)
		{
			try
			{
				res->try_expire();
			}
			catch(const std::runtime_error& ex)
			{
			}
		}
	}

	//! Removes all expired
	void remove_expired()
	{
		for (auto &[id, res] : m_resources)
			if (res->can_delete())
				remove(id);
	}

	//! Removes unused resources
	void garbage_collect()
	{
		expire_unused();
		remove_expired();
	}

	//! Attempts to remove/free a resource by ID
	void remove(id_type id)
	{
		std::scoped_lock lock{m_names_mutex};
		auto it = m_resources.find(id);

		if (it == m_resources.end())
			throw std::runtime_error{std::string{"resource_manager<T>::remove() called with UID="} + std::to_string(id)};

		if (!it->second->can_delete())
			throw std::runtime_error{std::string{"resource_manager<T>::remove() resource still in use UID="} + std::to_string(id)};

		m_resources.erase(it);
		auto name = get_name(id);
		m_name_manager.remove(name);
		m_ids.erase(name);
	}

	//! Attempts to remove/free a resource by name
	void remove(const std::string &name)
	{
		remove(get_id(name));
	}

	// Returns a resource handle by ID
	resource_handle<T> get(id_type id) const
	{
		auto it = m_resources.find(id);
		if (it == m_resources.end())
			throw std::runtime_error{std::string{"resource_manager<T>::get() called with UID="} + std::to_string(id)};
		return it->second->get_handle();
	}

	//! Returns resource handle by name
	resource_handle<T> get(const std::string &name) const
	{
		return get(get_id(name));
	}

	//! Returns resource ID by name
	id_type get_id(const std::string &name) const
	{
		std::scoped_lock lock{m_names_mutex};
		return m_ids.at(name);
	}

	//! Returns resource name by ID
	std::string get_name(id_type id) const
	{
		std::scoped_lock lock{m_names_mutex};
		for (const auto &[name, res_id] : m_ids)
			if (res_id == id)
				return name;
		throw std::out_of_range{std::string{"resource_manager::get_name() - no resource with ID="} + std::to_string(id)};
	}

	//! Returns a vector with all resource IDs
	std::vector<id_type> get_ids() const
	{
		std::vector<id_type> ids;
		ids.reserve(m_resources.size());
		for (const auto &[id, res] : m_resources)
			ids.push_back(id);
		return ids;
	}

protected:
	// std::string rename(const std::string &from, const std::string &to)
	// {
	// 	if (from == to) return from;

	// 	// TODO mutex lock
	// 	auto new_name = m_name_manager.rename(from, to);
	// 	m_ids[new_name] = m_ids.at(from);
	// 	m_ids.erase(from);
	// 	return new_name;
	// }

private:
	//! Maps IDs to resource pointers
	std::unordered_map<id_type, std::shared_ptr<T>> m_resources;
	
	//! Protects the name manager and ID map
	std::shared_mutex m_names_mutex;
	std::unordered_map<std::string, id_type> m_ids;
	bu::name_manager m_name_manager;
};

}