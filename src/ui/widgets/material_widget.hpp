#pragma once
#include <vector>
#include <memory>
#include "ui/widget.hpp"

namespace bu {
struct material_data;
}

namespace bu::ui {

class material_widget : public widget
{
public:
	material_widget(window &win) : 
		widget(win)
	{
	}

	void draw(const std::vector<std::shared_ptr<bu::material_data>> &materials);

private:
	void draw_editor(std::shared_ptr<bu::material_data> mat);

	std::weak_ptr<bu::material_data> m_selected;
};

}