#include "mouse.hpp"
#include "gl/gl.hpp"

using br::mouse_event_generator;

mouse_event_generator::mouse_event_generator(int buttons) :
	m_button_count(buttons),
	m_pos(0, 0),
	m_drag_start(buttons),
	m_drag_started(buttons),
	m_buttons(buttons),
	m_mods(buttons),
	m_last_events(buttons)
{
}

void mouse_event_generator::glfw_button_event(int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		m_drag_started.at(button) = false;
		m_drag_start.at(button) = m_pos;
		m_buttons.at(button) = true;
		m_mods.at(button) = mods;
	}
	else if (action == GLFW_RELEASE)
	{
		if (m_drag_started.at(button))
			m_last_events.at(button) = {mouse_event_type::DRAG_END, m_pos, m_drag_start.at(button), m_mods.at(button)};
		else
			m_last_events.at(button) = {mouse_event_type::CLICK, m_pos, m_pos, m_mods.at(button)};
		
		m_buttons.at(button) = false;
		m_drag_started.at(button) = false;
	}
}

void mouse_event_generator::glfw_position_event(double x, double y)
{
	m_pos = glm::vec2{x, y};

	// Check if mouse has moved when any button was pressed
	for (int i = 0; i < m_button_count; i++)
	{
		glm::vec2 drag = m_pos - m_drag_start.at(i);
		if (m_buttons.at(i) && glm::length(drag) > 0)
		{
			m_drag_started.at(i) = true;
			m_last_events.at(i) = {mouse_event_type::DRAG_START, m_pos, m_drag_start.at(i), m_mods.at(i)};
		}
	}
}

void mouse_event_generator::glfw_scroll_event(double x, double y)
{
	m_scroll += glm::vec2{x, y};
}

void mouse_event_generator::clear_last_event(int button)
{
	m_last_events.at(button).type = mouse_event_type::NONE;
}

void mouse_event_generator::clear_events()
{
	for (int i = 0; i < m_button_count; i++)
		clear_last_event(i);
}

void mouse_event_generator::clear_scroll()
{
	m_scroll = glm::vec2{0};
}

void mouse_event_generator::clear()
{
	clear_events();
	clear_scroll();
}

const glm::vec2 &mouse_event_generator::get_position() const
{
	return m_pos;
}

const glm::vec2 &mouse_event_generator::get_scroll() const
{
	return m_scroll;
}

bool mouse_event_generator::is_drag_pending(int button) const
{
	return m_drag_started.at(button);
}

int mouse_event_generator::get_drag_mods(int button) const
{
	return m_mods.at(button);
}

glm::vec2 mouse_event_generator::get_drag_start(int button) const
{
	return is_drag_pending(button) ? m_drag_start.at(button) : m_pos;
}

glm::vec2 mouse_event_generator::get_drag_delta(int button) const
{
	return m_pos - get_drag_start(button);
}

const br::mouse_event &mouse_event_generator::get_last_event(int button) const
{
	return m_last_events.at(button);
}