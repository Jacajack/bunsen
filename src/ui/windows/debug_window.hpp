#pragma once
#include "ui/window.hpp"
#include "resmgr/resource_manager.hpp"
#include <string>

namespace bu {
struct bunsen_editor;
}

namespace bu::ui {

struct test_data
{
	test_data(std::string d) : data(d) {}
	std::string data;
};

using test_resource = mrsw_resource<test_data>;


class debug_window : public bu::ui::window
{
public:
	debug_window(bunsen_editor &editor) :
		window("Evil Debug Cheats"),
		m_editor(editor)
	{}

	void draw() override;

private:
	float m_color[3] = {0};
	bunsen_editor &m_editor;
	resource_manager<test_resource> m_mgr;
};

}