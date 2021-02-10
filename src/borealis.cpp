#include "borealis.hpp"

#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gl/gl.hpp"
#include "camera.hpp"
#include "ui.hpp"
#include "preview/preview.hpp"
#include "assimp_loader.hpp"
#include "mouse.hpp"

static void glfw_error_callback(int error, const char *message)
{
	std::cerr << "GLFW error - " << message << std::endl;
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

static void glfw_cursor_position_callback(GLFWwindow *window, double x, double y)
{
	auto main_state = reinterpret_cast<br::borealis_state*>(glfwGetWindowUserPointer(window));
	main_state->mouse.glfw_position_event(x, y);
}

static void glfw_scroll_callback(GLFWwindow *window, double x, double y)
{
	auto main_state = reinterpret_cast<br::borealis_state*>(glfwGetWindowUserPointer(window));
	main_state->mouse.glfw_scroll_event(x, y);
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto main_state = reinterpret_cast<br::borealis_state*>(glfwGetWindowUserPointer(window));
	main_state->mouse.glfw_button_event(button, action, mods);
}

void main_loop(br::borealis_state &main_state)
{
	// TEMP
	br::scene scene;
	main_state.current_scene = &scene;
	br::preview_renderer preview;
	br::camera_orbiter orbiter;

	scene.meshes.emplace_back(br::load_mesh_from_file("resources/monkey.obj"));
	scene.objects.emplace_back();
	// scene.objects[0].transform

	while (!glfwWindowShouldClose(main_state.window))
	{
		main_state.mouse.clear();
		glfwPollEvents();
		
		// Get current window size
		glm::ivec2 window_size;
		glfwGetWindowSize(main_state.window, &window_size.x, &window_size.y);

		// Notify ImGui of new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Draw the scene
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window_size.x, window_size.y);

		// TEMP
		br::camera cam;
		cam.aspect = float(window_size.x) / window_size.y;
		br::update_camera_orbiter_from_mouse(orbiter, main_state.mouse, window_size);
		orbiter.update_camera(cam);
		preview.draw(*main_state.current_scene, cam);

		// Render ImGui
		br::draw_ui(main_state);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(main_state.window);
	}
}

int main(int argc, char *argv[])
{
	// Main state
	br::borealis_state main_state;

	// GLFW setup
	glfwSetErrorCallback(glfw_error_callback);
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// GLFW window creation
	main_state.window = glfwCreateWindow(720, 480, "Borealis", nullptr, nullptr);
	if (main_state.window == nullptr) throw std::runtime_error("glfwCreateWindow() failed!");
	glfwSetWindowUserPointer(main_state.window, &main_state);

	// GLFW callbacks
	glfwSetKeyCallback(main_state.window, glfw_key_callback);
	glfwSetCursorPosCallback(main_state.window, glfw_cursor_position_callback);
	glfwSetScrollCallback(main_state.window, glfw_scroll_callback);
	glfwSetMouseButtonCallback(main_state.window, glfw_mouse_button_callback);

	// Load GL functions
	glfwMakeContextCurrent(main_state.window);
	glewExperimental = true;
	if (glewInit() != GLEW_OK) throw std::runtime_error("glewInit() failed!");

	// Initialize ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	main_state.imgui_io = &ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(main_state.window, true);
	ImGui_ImplOpenGL3_Init();

	// Main loop
	try
	{
		main_loop(main_state);
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Main loop threw an exception - " << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Main loop threw an unrecognized exception..." << std::endl;
	}

	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// GLFW cleanup
	glfwTerminate();

	return 0;
}
