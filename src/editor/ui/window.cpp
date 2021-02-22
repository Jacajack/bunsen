#include "window.hpp"
#include <imgui.h>
#include "../../gl/gl.hpp"
#include "../../utils.hpp"
using bu::ui::window;

std::unordered_map<std::string, std::set<int>> window::m_instances;
bool window::m_mouse_locked = false;

window::window(const std::string &title, ImGuiWindowFlags flags) :
	m_flags(flags),
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
	if (ImGui::Begin(m_full_title.c_str(), &m_open, m_flags))
	{
		auto pos = bu::to_vec2(ImGui::GetWindowPos());
		auto min = pos + bu::to_vec2(ImGui::GetWindowContentRegionMin());
		auto max = pos + bu::to_vec2(ImGui::GetWindowContentRegionMax());
		auto mouse_pos = bu::to_vec2(ImGui::GetMousePos());
		bool mouse_inside = glm::all(glm::lessThanEqual(min, mouse_pos)) && glm::all(glm::lessThan(mouse_pos, max));
	
		if (m_owns_mouse && !mouse_inside)
		{
			auto win = glfwGetCurrentContext();
			double mx, my;
			glfwGetCursorPos(win, &mx, &my);
			
			glm::vec2 mouse_pos(mx, my);
			glm::vec2 new_pos(mx, my);

			if (mx < min.x)
				new_pos.x = max.x - 1;
			else if (mx >= max.x)
				new_pos.x = min.x;

			if (my < min.y)
				new_pos.y = max.y - 1;
			else if (my >= max.y) 
				new_pos.y = min.y;

			auto delta = new_pos - mouse_pos;
			m_mouse_offset -= delta;
			glfwSetCursorPos(win, new_pos.x, new_pos.y);
		}
	
		draw();

		if (!m_owns_mouse)
			m_mouse_offset = glm::vec2{0.f};
	}
	ImGui::End();
}

bool window::is_open() const
{
	return m_open;
}

bool window::can_lock_mouse() const
{
	return !m_mouse_locked || m_owns_mouse;
}

bool window::has_mouse() const
{
	return m_owns_mouse;
}

void window::lock_mouse()
{
	if (can_lock_mouse())
	{
		m_mouse_locked = true;
		m_owns_mouse = true;
	}
}

void window::release_mouse()
{
	if (m_owns_mouse)
	{
		m_owns_mouse = false;
		m_mouse_locked = false;
	}
}

window::~window()
{
	m_instances[m_title].erase(m_instance);
	release_mouse();
}