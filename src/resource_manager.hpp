#pragma once
#include <type_traits>
#include <memory>
#include <map>
#include <unordered_map>
#include "uid_provider.hpp"

namespace bu {

template <typename T>
struct managed_resource : public std::enable_shared_from_this<T>, public bu::uid_provider<T>
{
};

template <typename T>
class resource_manager
{
	static_assert(std::is_base_of_v<bu::managed_resource<T>, T>,
		"Resource manager can only manage classes derived from bu::managed_resource<T>");

public:
	using id_type = typename bu::uid_provider<T>::id_type;

	id_type add(std::shared_ptr<T> ptr, const std::string &name = {});
	id_type add(T &res, const std::string &name = {});

	void remove(id_type id);
	void remove(const std::string &name);

	std::shared_ptr<T> get(id_type id) const;
	std::shared_ptr<T> get(const std::string &name) const;
	id_type get_id_by_name(const std::string &name) const;

private:
	std::unordered_map<id_type, std::shared_ptr<T>> m_resources;
	std::map<std::string, id_type> m_names;
};

}