#pragma once

#include "gl/gl.hpp"
#include "scene.hpp"
#include "mouse.hpp"
#include <imgui.h>

namespace br {

/**
	\brief State of the application
*/
struct borealis_state
{
	GLFWwindow *window;
	ImGuiIO *imgui_io;
	br::mouse_event_generator mouse;

	br::scene *current_scene;
	bool gl_debug;

};

}

