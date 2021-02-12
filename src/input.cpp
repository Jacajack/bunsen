#include "input.hpp"
#include "gl/gl.hpp"

using bu::input_event_queue;
using bu::input_event_type;
using bu::input_event;

input_event_queue::input_event_queue(int buttons) :
	m_button_count(buttons),
	m_drag_start(buttons),
	m_drag_started(buttons),
	m_buttons(buttons),
	m_mods(buttons)
{
}

void input_event_queue::glfw_keyboard_event(int key, int action, int mods)
{
	if (m_inh_keyboard) return;
	m_queue.push_back({input_event_type::KEYBOARD, key, action, mods});
}

void input_event_queue::glfw_button_event(int button, int action, int mods)
{
	if (m_inh_mouse) return;
	m_queue.push_back({input_event_type::MOUSE_BUTTON, button, action, mods});

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
			m_queue.push_back({input_event_type::DRAG_END, button, GLFW_RELEASE, m_mods.at(button), m_pos, m_drag_start.at(button)});
		else
			m_queue.push_back({input_event_type::CLICK, button, GLFW_RELEASE, m_mods.at(button), m_pos, m_pos});
		
		m_buttons.at(button) = false;
		m_drag_started.at(button) = false;
	}
}

void input_event_queue::glfw_position_event(double x, double y)
{
	if (m_inh_mouse) return;
	m_pos = glm::vec2{x, y};
	m_queue.push_back({input_event_type::MOUSE_MOVE, 0, 0, 0, m_pos});

	// Check if mouse has moved when any button was pressed
	for (int i = 0; i < m_button_count; i++)
	{
		glm::vec2 drag = m_pos - m_drag_start.at(i);
		if (m_buttons.at(i) && glm::length(drag) > 0)
		{
			m_drag_started.at(i) = true;
			m_queue.push_back({input_event_type::DRAG_START, i, GLFW_PRESS, m_mods.at(i), m_pos, m_drag_start.at(i)});
		}
	}
}

void input_event_queue::glfw_scroll_event(double x, double y)
{
	if (m_inh_mouse) return;
	m_queue.push_back({input_event_type::MOUSE_SCROLL, 0, 0, 0, glm::vec2{x, y}});
	m_scroll += glm::vec2{x, y};
}

void input_event_queue::set_inhibit_mouse(bool inh)
{
	m_inh_mouse = inh;
}

void input_event_queue::set_inhibit_keyboard(bool inh)
{
	m_inh_keyboard = inh;
}

const input_event &input_event_queue::get_last_event() const
{
	return m_queue.back();
}

const std::vector<input_event> &input_event_queue::get_queue() const
{
	return m_queue;
}

void input_event_queue::clear_queue()
{
	m_queue.clear();
}

// -- Position and scroll

const glm::vec2 &input_event_queue::get_position() const
{
	return m_pos;
}

const glm::vec2 &input_event_queue::get_scroll() const
{
	return m_scroll;
}

void input_event_queue::clear_scroll()
{
	m_scroll = glm::vec2{0};
}

// --- DRAG

bool input_event_queue::is_drag_pending(int button) const
{
	return m_drag_started.at(button);
}

int input_event_queue::get_drag_mods(int button) const
{
	return m_mods.at(button);
}

glm::vec2 input_event_queue::get_drag_start(int button) const
{
	return is_drag_pending(button) ? m_drag_start.at(button) : m_pos;
}

glm::vec2 input_event_queue::get_drag_delta(int button) const
{
	return m_pos - get_drag_start(button);
}