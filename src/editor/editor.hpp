#pragma once
#include <set>
#include <list>
#include <memory>
#include "../bunsen.hpp"
#include "ui/window.hpp"

namespace bu {

struct scene;

struct bunsen_editor
{
	bunsen_editor(bu::scene *scene);
	void draw(const bu::bunsen &main_state);

	bu::scene *scene = nullptr;
	std::list<std::unique_ptr<ui::window>> windows;
};

}