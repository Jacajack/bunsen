#include "window.hpp"
#include <imgui.h>
#include "gl/gl.hpp"
#include "utils.hpp"
#include "event.hpp"
using bu::ui::window;

std::unordered_map<std::string, std::vector<bool>> window::m_instances;
bool window::m_mouse_locked = false;

window::window(const std::string &title, ImGuiWindowFlags flags, std::shared_ptr<bu::event_bus> bus) :
	m_flags(flags),
	m_title(title)
{
	auto &ids = m_instances[title];

	// First ID is 0
	int my_id = -1;

	// Find first unused
	for (auto i = 0u; i < ids.size(); i++)
	{
		if (!ids[i])
		{
			ids[i] = true;
			my_id = i;
			break;
		}
	}

	// Add new ID slot
	if (my_id < 0)
	{
		my_id = ids.size();
		ids.push_back(true);
	}

	m_instance = my_id;


	if (m_instance == 0)
		m_full_title = m_title;
	else
		m_full_title = m_title + " [" + std::to_string(m_instance) + "]";

	// Event bus connection
	if (bus)
		m_events = bus->make_connection();
}

void window::display()
{
	update();

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
			ImGui::GetIO().MousePos = ImVec2(new_pos.x, new_pos.y);
		}
		
		if (!m_owns_mouse)
			m_mouse_offset = glm::vec2{0, 0};
	
		draw();

	}
	ImGui::End();

	m_events->clear();
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
	if (can_lock_mouse() && !has_mouse())
	{
		m_mouse_locked = true;
		m_owns_mouse = true;
		m_mouse_offset = glm::vec2{0.f};
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

std::shared_ptr<bu::event_bus> window::get_event_bus() const
{
	if (!m_events)
		throw std::runtime_error{
			"window::get_event_bus() called on window without"
			" a connection to the event bus!"
			};
	return m_events->get_bus();
}

window::~window()
{
	m_instances[m_title].at(m_instance) = false;
	release_mouse();
}