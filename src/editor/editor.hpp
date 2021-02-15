#pragma once
#include "../bunsen.hpp"
#include "../scene.hpp"
#include "../camera.hpp"
#include "../renderers/preview/preview.hpp"
#include "layout_editor.hpp"
#include "imgui_overlay.hpp"
#include <memory>
#include <list>

namespace bu {

struct bunsen_editor
{
	using scene_selection = std::list<std::weak_ptr<bu::scene_node>>;
	scene_selection selected_nodes;

	// Layout editor
	bu::layout_editor layout_ed;
	bu::imgui_overlay overlay;

	// Current scene
	bu::scene *scene = nullptr;

	// Camera manipulation
	bu::camera viewport_camera;
	bu::camera_orbiter orbit_manipulator;
	
	// Renderers
	std::unique_ptr<bu::preview_renderer> preview = std::make_unique<bu::preview_renderer>();

	void draw(const bu::bunsen_state &main_state);
};

}