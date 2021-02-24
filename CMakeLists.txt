project(bunsen)
cmake_minimum_required(VERSION 3.3)

option(GL_DEBUG "Should OpenGL debugging be enabled? (meaningless in Debug builds)" OFF)
option(SANITIZE "Should sanitizer be enabled in debug builds? (can conflict with debuggers)" ON)

set(OpenGL_GL_PREFERENCE GLVND)

include(cmake/get_imgui.cmake)
include(cmake/get_imgui_file_dialog.cmake)
include(cmake/get_imgui_icon_font_headers.cmake)
include(cmake/get_tracy.cmake)
include(cmake/get_inih.cmake)

find_package(glm 0.9.9 REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL 4.2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ASSIMP REQUIRED)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter --std=c++17 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-fno-omit-frame-pointer -g -DDEBUG -ffast-math -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -ftree-vectorize -march=native -DNDEBUG")

if (SANITIZE)
	message("Sanitizers will be enabled!")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
endif()

add_executable(bunsen 
	"src/bunsen.cpp"
	"src/config.cpp"
	"src/utils.cpp"
	"src/camera.cpp"
	"src/log.cpp"
	"src/scene.cpp"
	"src/mesh.cpp"
	"src/input.cpp"
	"src/assimp_loader.cpp"
	"src/scene_export.cpp"
	"src/scene_selection.cpp"

	"src/gl/gl.cpp"
	"src/gl/shader.cpp"

	"src/renderers/preview/preview.cpp"

	"src/renderers/rt/rt.cpp"
	"src/renderers/rt/worker.cpp"
	"src/renderers/rt/sampled_image.cpp"

	"src/renderers/albedo/albedo.cpp"

	"src/editor/editor.cpp"
	"src/editor/layout_editor.cpp"
	"src/editor/imgui_overlay.cpp"

	"src/editor/ui/ui.cpp"
	"src/editor/ui/window.cpp"
	"src/editor/ui/scene_control.cpp"
	"src/editor/ui/material_menu.cpp"
	"src/editor/ui/light_menu.cpp"
	"src/editor/ui/model_menu.cpp"
	"src/editor/ui/world_menu.cpp"
	"src/editor/ui/rendered_view.cpp"
	)

target_link_libraries(bunsen PRIVATE 
	glm
	glfw
	GLEW
	OpenGL::GL
	nlohmann_json::nlohmann_json
	${ASSIMP_LIBRARIES}
	imgui_glfw
	imgui_file_dialog
	imgui_icon_font_headers
	tracy
	inih
	)

if (GL_DEBUG)
	target_compile_definitions(bunsen PRIVATE GL_DEBUG)
endif()

add_custom_target(
	symlink_resources ALL
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	"${PROJECT_SOURCE_DIR}/resources" "${PROJECT_BINARY_DIR}/resources"
	)