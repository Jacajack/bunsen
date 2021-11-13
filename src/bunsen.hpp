#pragma once
#include <memory>
#include "gl/gl.hpp"
#include "config.hpp"
#include "resmgr/resource_engine.hpp"

struct ImGuiIO;

namespace bu {

struct resource_engine;

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

	// TODO setters/getters
	GLFWwindow *window;
	ImGuiIO *imgui_io;
	bu::bunsen_config config;
	bool gl_debug;
	bool debug;

	std::unique_ptr<bu::resource_engine> res_engine;

private:
	bunsen() = default;
	static std::unique_ptr<bunsen> instance_ptr;
};

static inline bunsen &ctx()
{
	return bunsen::get();
}

static inline resource_engine &res()
{
	return *bu::ctx().res_engine;
}

}

