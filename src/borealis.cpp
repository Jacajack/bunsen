#include "borealis.hpp"

#include <iostream>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gl.hpp"
#include "ui.hpp"

void glfw_error_callback(int error, const char *message)
{
	std::cerr << "GLFW error - " << message << std::endl;
}

void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

}

void main_loop(br::borealis_state &main_state)
{
	while (!glfwWindowShouldClose(main_state.window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Notify ImGui of new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

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
	glfwWindowHint(GLFW_SAMPLES, 0);
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
