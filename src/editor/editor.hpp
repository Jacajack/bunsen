#pragma once
#include <set>
#include <list>
#include <memory>
#include "../bunsen.hpp"
#include "../renderers/preview/preview.hpp"
#include "../renderers/albedo/albedo.hpp"
#include "../renderers/rt/rt.hpp"
#include "ui/window.hpp"

namespace bu {

struct scene;

struct bunsen_editor
{
	bunsen_editor();
	void draw(const bu::bunsen &main_state);

	std::shared_ptr<bu::scene> scene;
	std::list<std::unique_ptr<ui::window>> windows;

	// Rendering contexts shared by all open viewports
	std::shared_ptr<preview_context> preview_ctx;
	std::shared_ptr<albedo_context> albedo_ctx;
	std::shared_ptr<rt_context> rt_ctx;
};

}