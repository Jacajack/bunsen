#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <imgui.h>
#include <glm/glm.hpp>

namespace bu::ui {

/**
	\brief A base class for all ImGui windows
*/
class window
{
public:
	window(const std::string &title, ImGuiWindowFlags flags = 0);
	void display();
	bool is_open() const;

	virtual ~window();

protected:
	virtual void draw() = 0;
	virtual void update() {}
	bool can_lock_mouse() const;
	bool has_mouse() const;
	void lock_mouse();
	void release_mouse();


	bool m_open = true;
	ImGuiWindowFlags m_flags;
	std::string m_full_title;
	glm::vec2 m_mouse_offset;

private:
	bool m_owns_mouse = false;
	int m_instance;
	std::string m_title;

	static std::unordered_map<std::string, std::set<int>> m_instances;
	static bool m_mouse_locked;
};

}