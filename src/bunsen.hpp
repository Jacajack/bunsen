#pragma once
#include <memory>
#include "gl/gl.hpp"
#include "config.hpp"

struct ImGuiIO;

namespace bu {

/**
	\brief Application instance - a singleton
*/
class bunsen
{
public:
	static bunsen &get();
	static void destroy();
	
	bunsen(const bunsen &) = delete;
	bunsen(bunsen &&) = delete;
	~bunsen() = default;

	GLFWwindow *window;
	ImGuiIO *imgui_io;
	bu::bunsen_config config;
	bool gl_debug;
	bool debug;

private:
	bunsen() = default;
	static std::unique_ptr<bunsen> instance_ptr;
};

}

