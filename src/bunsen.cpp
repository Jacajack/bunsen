#include "bunsen.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <thread>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>
#include <tracy/TracyOpenGL.hpp>

#include "gl/gl.hpp"
#include "ui/editor.hpp"
#include "ui/ui.hpp"
#include "config.hpp"
#include "log.hpp"
#include "async_task.hpp"

using bu::bunsen;

std::unique_ptr<bu::bunsen> bunsen::instance_ptr;

bunsen &bunsen::get()
{
	if (instance_ptr)
		return *instance_ptr;
	else
	{
		instance_ptr = std::unique_ptr<bunsen>(new bunsen);
		return *instance_ptr;
	}
}

void bunsen::destroy()
{
	instance_ptr.reset();
}

static void glfw_error_callback(int error, const char *message)
{
	LOG_ERROR << "GLFW error - " << message;
}

void main_loop(bu::bunsen &main_state)
{
	// The main editor and scene
	bu::bunsen_editor main_editor;

	TracyGpuContext;

	while (!glfwWindowShouldClose(main_state.window))
	{
		// Wait for events
		glfwWaitEventsTimeout(1.0 / 30.0);

		// Notify ImGui of new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		// Get current window size
		glm::ivec2 window_size;
		glfwGetWindowSize(main_state.window, &window_size.x, &window_size.y);

		// --- GL debug start
		if (main_state.gl_debug)
			glEnable(GL_DEBUG_OUTPUT);

		// Clear window and set proper viewport size
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window_size.x, window_size.y);

		// The main editor
		TracyCZoneN(zone_editor, "Editor draw", true);
		main_editor.draw(main_state);
		TracyCZoneEnd(zone_editor);

		// --- GL debug end
		if (main_state.gl_debug)
			glDisable(GL_DEBUG_OUTPUT);

		// Render ImGui
		TracyCZoneN(zone_imgui, "ImGui Render", true);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		TracyCZoneEnd(zone_imgui);

		glfwSwapBuffers(main_state.window);
		FrameMark;
		TracyGpuCollect;
	}
}

static void task_cleaner_thread(std::atomic<bool> *active)
{
	using namespace std::chrono_literals;

	while (*active)
	{
		while (!bu::global_task_cleaner.empty())
			bu::global_task_cleaner.collect();
		std::this_thread::sleep_for(100ms);
	}

	// Make sure all are cleaned
	while (!bu::global_task_cleaner.empty())
		bu::global_task_cleaner.collect();

	LOG_DEBUG << "Task cleaner thread terminating...";
}

int main(int argc, char *argv[])
{
	// Main state
	auto &main_state = bu::bunsen::get();

	// Debug announcement
	LOG_DEBUG << "Debug output is enabled!";

	// Change directory to executable dir
	std::filesystem::path exec_path(argv[0]);
	std::filesystem::current_path(exec_path.parent_path());
	LOG_INFO << "Working directory: " << std::filesystem::current_path();

	main_state.gl_debug = false;
	#if defined(GL_DEBUG) || defined(DEBUG)
	main_state.gl_debug = true;
	#endif

	main_state.debug = false;
	#if defined(DEBUG) || defined(BUNSEN_DEBUG)
	main_state.debug = true;
	#endif

	// Start task cleaner
	std::atomic<bool> task_cleaner_active = true;
	auto task_cleaner = std::async(std::launch::async, task_cleaner_thread, &task_cleaner_active);

	// Read config
	const char *default_config_path = "bunsen.ini";
	if (bu::read_config_from_file(main_state.config, default_config_path))
		LOG_INFO << "Found and read default config file (" << default_config_path << ")";
	else
		LOG_WARNING << "Failed to read default config file (" << default_config_path << ") - assuming defaults";

	// Get window config
	auto initial_resx = main_state.config.general.resx;
	auto initial_resy = main_state.config.general.resy;
	auto msaa = main_state.config.general.msaa;

	// GLFW setup
	glfwSetErrorCallback(glfw_error_callback);
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, msaa);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, main_state.gl_debug ? GLFW_TRUE : GLFW_FALSE);

	// GLFW window creation
	main_state.window = glfwCreateWindow(initial_resx, initial_resy, "bunsen", nullptr, nullptr);
	if (main_state.window == nullptr)
	{
		LOG_ERROR << "glfwCreateWindow() failed! Terminating...";
		return 1; 
	}
	glfwSetWindowUserPointer(main_state.window, &main_state);

	// Load GL functions
	glfwMakeContextCurrent(main_state.window);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		LOG_ERROR << "glewInit() failed! Terminating...";
		return 1;
	}

	// Setup OpenGL debug callback
	if (main_state.gl_debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glDebugMessageCallback(bu::gl_debug_callback, &main_state);
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0,
			GL_DEBUG_SEVERITY_LOW, -1, "OpenGL debug output is active");
	}
	
	// Initialize ImGui context and load fonts
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	main_state.imgui_io = &ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(main_state.window, true);
	ImGui_ImplOpenGL3_Init();
	main_state.imgui_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;    
	bu::ui::load_extra_fonts(*main_state.imgui_io);

	// Load theme color from config and set ImGui theme
	{
		auto &thm = main_state.config.theme;
		bu::ui::load_theme(thm.r, thm.g, thm.b);
	}

	LOG_INFO << "Init completed!";

	// Main loop
	try
	{
		main_loop(main_state);
	}
	catch (const std::exception &ex)
	{
		LOG_ERROR << "Main loop threw an exception - " << ex.what() << std::endl;
	}
	catch (...)
	{
		LOG_ERROR << "Main loop threw an unrecognized exception..." << std::endl;
	}

	LOG_INFO << "Waiting for background tasks to complete...";

	// Shut down task cleaner
	task_cleaner_active = false;
	task_cleaner.wait();
	LOG_INFO << "Shutting down!";

	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// GLFW cleanup
	glfwTerminate();
	bu::bunsen::destroy();
	return 0;
}
