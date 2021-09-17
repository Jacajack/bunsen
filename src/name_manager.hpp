#pragma once
#include <string>
#include <set>

namespace bu {

/**
	\brief Ensures uniqueness of stored names
*/
class name_manager
{
public:
	std::string add(const std::string &name);
	std::string rename(const std::string &from, const std::string &to);
	void remove(const std::string &name);

private:
	using indexed_name = std::pair<std::string, int>;
	indexed_name split_name(const std::string &name) const;
	std::string full_name(const indexed_name &iname) const;

	std::set<indexed_name> m_names;
};

}