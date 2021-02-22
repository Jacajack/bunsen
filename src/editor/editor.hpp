#pragma once
#include <set>
#include <list>
#include <memory>
#include "../bunsen.hpp"
#include "../renderers/preview/preview.hpp"
#include "ui/window.hpp"

namespace bu {

struct scene;

struct bunsen_editor
{
	bunsen_editor();
	void draw(const bu::bunsen &main_state);

	std::shared_ptr<bu::scene> scene;
	std::list<std::unique_ptr<ui::window>> windows;

	// Renderer instance shared by all the viewports
	std::shared_ptr<bu::renderer> default_renderer;
	std::shared_ptr<bu::preview_renderer> preview_renderer;
};

}