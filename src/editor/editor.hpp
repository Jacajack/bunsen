#pragma once
#include <set>
#include <list>
#include <memory>
#include "../bunsen.hpp"
#include "ui/window.hpp"

namespace bu {

struct bunsen_editor
{
	bunsen_editor(bu::scene *scene);
	void draw(const bu::bunsen_state &main_state);

	bu::scene *scene = nullptr;
	std::list<std::unique_ptr<ui::window>> windows;
};

}