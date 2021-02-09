#pragma once

#include "gl.hpp"
#include <imgui.h>

namespace br {

/**
	\brief State of the application
*/
struct borealis_state
{
	GLFWwindow *window;
	ImGuiIO *imgui_io;
};

}

