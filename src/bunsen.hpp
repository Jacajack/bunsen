#pragma once

#include "gl/gl.hpp"
#include "scene.hpp"
#include "input.hpp"
#include <imgui.h>
#include <INIReader.h>

namespace bu {

/**
	\brief State of the application
*/
struct bunsen_state
{
	GLFWwindow *window;
	ImGuiIO *imgui_io;
	bu::input_event_queue user_input;
	INIReader *config;
	
	bool gl_debug;
	bool debug;
};

}

