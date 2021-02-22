#include "window.hpp"
#include <imgui.h>

using bu::ui::window;

std::unordered_map<std::string, std::set<int>> window::m_instances;

window::window(const std::string &title) :
	m_title(title)
{
	int max = -1;
	auto &ids = m_instances[title];
	for (auto &id : ids)
		max = std::max(max, id);
	m_instance = max + 1;
	ids.insert(m_instance);

	if (m_instance == 0)
		m_full_title = m_title;
	else
		m_full_title = m_title + " [" + std::to_string(m_instance) + "]";
}

void window::display()
{
	if (ImGui::Begin(m_full_title.c_str(), &m_open))
		draw();
	ImGui::End();
}

bool window::is_open() const
{
	return m_open;
}

window::~window()
{
	m_instances[m_title].erase(m_instance);
}