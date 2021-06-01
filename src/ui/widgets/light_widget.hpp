#pragma once
#include <memory>
#include "ui/widget.hpp"

namespace bu {
struct light;
}

namespace bu::ui {

class light_widget : public widget
{
public:
	light_widget(window &win) :
		widget(win)
	{
	}

	void draw(std::shared_ptr<bu::light> light);
};

}