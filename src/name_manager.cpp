#include "name_manager.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <regex>

using bu::name_manager;

std::string name_manager::add(const std::string &name)
{
	if (name.empty())
		throw std::runtime_error{"name_manager - added empty name!"};

	auto p = split_name(name);
	if (m_names.find(p) == m_names.end())
	{
		m_names.insert(p);
		return name;
	}
	else
	{
		
		for (int i = 1; i < 1000; i++)
		{
			p.second = i;
			if (m_names.find(p) == m_names.end())
			{
				m_names.insert(p);
				return full_name(p);
			}
		}

		throw std::runtime_error{"name_manager - index overflow"};
	}
}

std::string name_manager::rename(const std::string &from, const std::string &to)
{
	if (from == to) return from;
	auto new_name = add(to);
	remove(from);
	return new_name;
}

void name_manager::remove(const std::string &name)
{
	m_names.erase(split_name(name));
}

name_manager::indexed_name name_manager::split_name(const std::string &name) const
{
	if (name.empty())
		return {"unnamed", -1};

	std::regex reg("^(.*)\\.([0-9]+)$");
	std::smatch match;
	if (std::regex_search(name, match, reg))
	{
		auto prefix = match[1];
		auto id_str = match[2];
		int instance = std::stoi(id_str);
		if (instance >= 0)
			return {prefix, instance};
	}

	return {name, -1};
}

std::string name_manager::full_name(const indexed_name &iname) const
{
	if (iname.second >= 0)
	{
		std::ostringstream ss;
		ss << iname.first << "." << std::setw(3) << std::setfill('0') << iname.second;
		return ss.str();
	}
	else
		return iname.first;
}