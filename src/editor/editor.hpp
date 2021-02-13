#pragma once
#include "../bunsen.hpp"
#include "../scene.hpp"
#include "../camera.hpp"
#include "../renderers/preview/preview.hpp"
#include <memory>

namespace bu {

struct bunsen_editor
{
	enum class editor_action
	{
		NONE,
		
		GRAB,
		GRAB_X,
		GRAB_Y,
		GRAB_Z,

		ROTATE,
		ROTATE_X,
		ROTATE_Y,
		ROTATE_Z,

		SCALE,
		SCALE_X,
		SCALE_Y,
		SCALE_Z,
	};

	editor_action current_action = editor_action::NONE;
	void *scene_selection = nullptr;

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