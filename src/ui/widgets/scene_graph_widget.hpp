#pragma once
#include <vector>
#include <memory>
#include "ui/widget.hpp"

namespace bu {
struct scene;
class scene_selection;
}

namespace bu::ui {

class scene_graph_widget : public widget
{
public:
	scene_graph_widget(window &win) : 
		widget(win)
	{
	}

	void draw(const bu::scene &scene, bu::scene_selection &selection);

private:

};

}