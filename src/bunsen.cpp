#include "bunsen.hpp"

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <filesystem>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <INIReader.h>

#include "gl/gl.hpp"
#include "input.hpp"
#include "editor/editor.hpp"
#include "editor/ui/ui.hpp"
#include "log.hpp"

static void glfw_error_callback(int error, const char *message)
{
	LOG_ERROR << "GLFW error - " << message;
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	auto main_state = reinterpret_cast<bu::bunsen_state*>(glfwGetWindowUserPointer(window));
	main_state->user_input.glfw_keyboard_event(key, action, mods);
}

static void glfw_cursor_position_callback(GLFWwindow *window, double x, double y)
{
	auto main_state = reinterpret_cast<bu::bunsen_state*>(glfwGetWindowUserPointer(window));
	main_state->user_input.glfw_position_event(x, y);
}

static void glfw_scroll_callback(GLFWwindow *window, double x, double y)
{
	auto main_state = reinterpret_cast<bu::bunsen_state*>(glfwGetWindowUserPointer(window));
	main_state->user_input.glfw_scroll_event(x, y);
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto main_state = reinterpret_cast<bu::bunsen_state*>(glfwGetWindowUserPointer(window));
	main_state->user_input.glfw_button_event(button, action, mods);
}

void main_loop(bu::bunsen_state &main_state)
{
	// The main editor and scene
	bu::scene main_scene;
	bu::bunsen_editor main_editor;
	main_editor.scene = &main_scene;

	while (!glfwWindowShouldClose(main_state.window))
	{
		// Clear input queue and wait for events
		main_state.user_input.clear_queue();
		main_state.user_input.clear_scroll();
		main_state.user_input.set_inhibit_mouse(main_state.imgui_io->WantCaptureMouse);
		main_state.user_input.set_inhibit_keyboard(main_state.imgui_io->WantCaptureKeyboard);
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window_size.x, window_size.y);

		// The main editor
		main_editor.draw(main_state);

		// --- GL debug end
		if (main_state.gl_debug)
			glDisable(GL_DEBUG_OUTPUT);

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(main_state.window);
	}
}

int main(int argc, char *argv[])
{
	// Main state
	bu::bunsen_state main_state;

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
	#if defined(DEBUG)
	main_state.debug = true;
	#endif

	// Read config
	const char *default_config_path = "bunsen.ini";
	auto ini_config = std::make_unique<INIReader>(default_config_path);
	if (ini_config->ParseError() < 0)
	{
		LOG_WARNING << "Failed to read default config file (" << default_config_path << ")";
		std::string default_config = 
			"[general]"
			"config_missing = true"
			;

		ini_config = std::make_unique<INIReader>(default_config.c_str(), default_config.length());
		if (ini_config->ParseError() < 0)
		{
			LOG_ERROR << "The default generated config is invalid!";
		}
	}
	else
	{
		LOG_INFO << "Found and read default config file (" << default_config_path << ")";
	}
	main_state.config = ini_config.get();

	// Get window config
	auto initial_resx = main_state.config->GetInteger("general", "resx", 1280);
	auto initial_resy = main_state.config->GetInteger("general", "resy", 720);
	auto msaa = main_state.config->GetInteger("general", "msaa", 2);

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

	// GLFW callbacks
	glfwSetKeyCallback(main_state.window, glfw_key_callback);
	glfwSetCursorPosCallback(main_state.window, glfw_cursor_position_callback);
	glfwSetScrollCallback(main_state.window, glfw_scroll_callback);
	glfwSetMouseButtonCallback(main_state.window, glfw_mouse_button_callback);

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
	bu::ui::load_theme();

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

	LOG_INFO << "Terminating...";

	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// GLFW cleanup
	glfwTerminate();

	return 0;
}
