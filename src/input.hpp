#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include "gl/gl.hpp"

namespace bu {

enum class input_event_type
{
	KEYBOARD,
	MOUSE_MOVE,
	MOUSE_BUTTON,
	MOUSE_SCROLL,
	CLICK,
	DRAG_START,
	DRAG_END
};

struct input_event
{
	input_event_type type;
	int key;
	int action;
	int mods;
	glm::vec2 position = glm::vec2{0.f};
	glm::vec2 start_position = glm::vec2{0.f};
};

class input_event_queue
{
public:
	input_event_queue(int buttons = GLFW_MOUSE_BUTTON_LAST);

	// GLFW inputs
	void glfw_keyboard_event(int key, int action, int mods);
	void glfw_button_event(int button, int action, int mods);
	void glfw_position_event(double x, double y);
	void glfw_scroll_event(double x, double y);
	
	// Inhibit inputs
	void set_inhibit_mouse(bool inh);
	void set_inhibit_keyboard(bool inh);

	// Event queue access
	const input_event &get_last_event() const;
	const std::vector<input_event> &get_queue() const;
	void clear_queue();

	// Mouse position and accumulated scroll
	const glm::vec2 &get_position() const;
	const glm::vec2 &get_scroll() const;
	void clear_scroll();

	// Mouse drag related
	bool is_drag_pending(int button) const;
	int get_drag_mods(int button) const;
	glm::vec2 get_drag_start(int button) const;
	glm::vec2 get_drag_delta(int button) const;

private:
	bool m_inh_mouse = false;
	bool m_inh_keyboard = false;
	int m_button_count;
	glm::vec2 m_pos = glm::vec2{0.f};
	glm::vec2 m_scroll = glm::vec2{0.f};
	std::vector<glm::vec2> m_drag_start;
	std::vector<bool> m_drag_started;
	std::vector<bool> m_buttons;
	std::vector<int> m_mods;
	std::vector<input_event> m_queue;
};

}