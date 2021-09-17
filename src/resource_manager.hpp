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

template <typename T>
class resource_manager
{
	static_assert(std::is_base_of_v<bu::resource<T>, T>,
		"Resource manager can only manage classes derived from bu::managed_resource<T>");

public:
	using id_type = typename bu::uid_provider<T>::id_type;

	resource_handle<T> add(std::unique_ptr<T> ptr, const std::string &name)
	{
		// Save resource pointer
		auto id = ptr->resource_id();
		std::shared_ptr<T> shared{std::move(ptr)};
		auto [it, insert_ok] = m_resources.insert({id, shared});
		auto &res = *it->second;

		if (!insert_ok)
			throw std::runtime_error{"resource_manager add() failed to insert"};

		// Register name (or suggest new)
		// TODO mutex lock
		std::string new_name = m_name_manager.add(name);
		m_ids[new_name] = id;

		// Init resource
		// res.resource_init(new_name, this);

		LOG_INFO << "added resource '" << new_name << "' (id=" << id << ")";

		return shared->get_handle();
	}

	void remove(const std::string &name)
	{
		remove(get_id(name));
	}

	resource_handle<T> get(id_type id) const
	{
		auto it = m_resources.find(id);
		if (it == m_resources.end())
			throw std::runtime_error{std::string{"resource_manager<T>::get() called with UID="} + std::to_string(id)};
		return it->second->get_handle();
	}

	resource_handle<T> get(const std::string &name) const
	{
		return get(get_id(name));
	}

	id_type get_id(const std::string &name) const
	{
		// TODO mutex lock
		return m_ids.at(name);
	}

	std::vector<id_type> get_ids() const
	{
		std::vector<id_type> ids;
		ids.reserve(m_resources.size());
		for (const auto &[id, res] : m_resources)
			ids.push_back(id);
		return ids;
	}

protected:
	std::string rename(const std::string &from, const std::string &to)
	{
		if (from == to) return from;

		// TODO mutex lock
		auto new_name = m_name_manager.rename(from, to);
		m_ids[new_name] = m_ids.at(from);
		m_ids.erase(from);
		return new_name;
	}

private:
	std::unordered_map<id_type, std::shared_ptr<T>> m_resources;
	
	std::shared_mutex m_names_mutex;
	std::unordered_map<std::string, id_type> m_ids;
	bu::name_manager m_name_manager;
};

template <typename T>
using mrsw_resource_manager = resource_manager<mrsw_resource<T>>;

}