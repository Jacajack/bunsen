#pragma once
#include <memory>
#include "ui/widget.hpp"

namespace bu {
class scene_node;
}

namespace bu::ui {

class node_widget : public widget
{
public:
	node_widget(window &win) :
		widget(win)
	{
	}

	void draw(std::shared_ptr<bu::scene_node> node);
};

}