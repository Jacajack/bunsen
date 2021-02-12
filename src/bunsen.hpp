#pragma once

#include "gl/gl.hpp"
#include "scene.hpp"
#include "mouse.hpp"
#include <imgui.h>

namespace bu {

/**
	\brief State of the application
*/
struct bunsen_state
{
	GLFWwindow *window;
	ImGuiIO *imgui_io;
	bu::mouse_event_generator mouse;

	bu::scene *current_scene;
	bool gl_debug;

};

}

