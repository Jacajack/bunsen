#pragma once
#include <vector>
#include <memory>
#include "ui/widget.hpp"
#include "resmgr/resource_engine.hpp"

namespace bu {
struct material_data;
}

namespace bu::ui {

class material_widget : public widget
{
public:
	material_widget(window &win) : 
		widget(win),
		m_selected(0)
	{
	}

	void draw(const std::vector<bu::resource_handle<bu::material_resource>> &materials);

private:
	void draw_editor(bu::resource_handle<bu::material_resource> mat);

	std::uint64_t m_selected;
};

}