#pragma once
#include <string>
#include <set>
#include <unordered_map>

namespace bu::ui {

/**
	\brief A base class for all ImGui windows
*/
class window
{
public:
	window(const std::string &title);
	void display();
	bool is_open() const;
	virtual ~window();

protected:
	virtual void draw() = 0;

	bool m_open = true;
	int m_instance;
	std::string m_title;
	std::string m_full_title;

	static std::unordered_map<std::string, std::set<int>> m_instances;
};

}