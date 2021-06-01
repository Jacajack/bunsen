#pragma once
#include <vector>
#include <memory>
#include "ui/widget.hpp"

namespace bu {
struct world;
}

namespace bu::ui {

class world_widget : public widget
{
public:
	world_widget(window &win) : 
		widget(win)
	{
	}

	void draw(bu::world &world);
};

}