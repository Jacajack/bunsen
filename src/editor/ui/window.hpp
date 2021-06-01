#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <imgui.h>
#include <glm/glm.hpp>
#include <memory>

namespace bu {
class event_bus;
class event_bus_connection;
}

namespace bu::ui {

/**
	\brief A base class for all ImGui windows
*/
class window
{
public:
	window(const std::string &title, ImGuiWindowFlags flags = 0, std::shared_ptr<bu::event_bus> bus = {});
	virtual ~window();

	void display();
	bool is_open() const;

protected:
	virtual void draw() = 0;
	virtual void update() {}
	bool can_lock_mouse() const;
	bool has_mouse() const;
	void lock_mouse();
	void release_mouse();
	bu::event_bus_connection &get_event_bus_connection();

	bool m_open = true;
	ImGuiWindowFlags m_flags;
	std::string m_full_title;
	glm::vec2 m_mouse_offset;

private:
	bool m_owns_mouse = false;
	int m_instance;
	std::string m_title;
	std::shared_ptr<bu::event_bus_connection> m_events;

	static std::unordered_map<std::string, std::vector<bool>> m_instances;
	static bool m_mouse_locked;
};

}