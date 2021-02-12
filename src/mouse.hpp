#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "gl/gl.hpp"

namespace bu {

enum class mouse_event_type
{
	NONE,
	CLICK,
	DRAG_START,
	DRAG_END
};

struct mouse_event
{
	mouse_event_type type = mouse_event_type::NONE;
	glm::vec2 position;
	glm::vec2 start_position;
	int mods;
};

class mouse_event_generator
{
public:
	mouse_event_generator(int buttons = GLFW_MOUSE_BUTTON_LAST);

	void glfw_button_event(int button, int action, int mods);
	void glfw_position_event(double x, double y);
	void glfw_scroll_event(double x, double y);
	void clear_last_event(int button);
	void clear_events();
	void clear_scroll();
	void clear();

	const glm::vec2 &get_position() const;
	const glm::vec2 &get_scroll() const;
	bool is_drag_pending(int button) const;
	int get_drag_mods(int button) const;
	glm::vec2 get_drag_start(int button) const;
	glm::vec2 get_drag_delta(int button) const;
	const mouse_event &get_last_event(int button) const;

private:
	int m_button_count;
	glm::vec2 m_pos;
	glm::vec2 m_scroll;
	std::vector<glm::vec2> m_drag_start;
	std::vector<bool> m_drag_started;
	std::vector<bool> m_buttons;
	std::vector<int> m_mods;
	std::vector<mouse_event> m_last_events;
};

}