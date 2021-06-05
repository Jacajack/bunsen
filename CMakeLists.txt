project(bunsen)
cmake_minimum_required(VERSION 3.3)

option(GL_DEBUG "Should OpenGL debugging be enabled? (meaningless in Debug builds)" OFF)
option(BUNSEN_DEBUG "Enable debug features in builds" OFF)
option(SANITIZE "Should sanitizer be enabled in debug builds? (can conflict with debuggers)" ON)
option(STRIP "Should the executable be stripped in release builds?" OFF)

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

if (STRIP)
	message("Stripping!")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
else()
	message("Embedding debugging information in the executable!")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
endif()

add_executable(bunsen 
	"src/bunsen.cpp"
	"src/config.cpp"
	"src/utils.cpp"
	"src/camera.cpp"
	"src/log.cpp"
	"src/event.cpp"
	"src/scene.cpp"
	"src/input.cpp"
	"src/assimp_loader.cpp"
	"src/scene_export.cpp"
	"src/scene_selection.cpp"

	"src/gl/gl.cpp"
	"src/gl/shader.cpp"

	"src/renderers/basic_gl_renderer.cpp"
	"src/renderers/preview/preview.cpp"
	"src/renderers/preview/basic_preview.cpp"

	"src/renderers/rt/rt.cpp"
	"src/renderers/rt/job.cpp"
	"src/renderers/rt/sampled_image.cpp"
	"src/renderers/rt/aabb.cpp"
	"src/renderers/rt/bvh_builder.cpp"
	"src/renderers/rt/bvh_populate.cpp"
	"src/renderers/rt/bvh.cpp"
	"src/renderers/rt/scene_cache.cpp"
	"src/renderers/rt/material.cpp"
	"src/renderers/rt/kernel.cpp"

	"src/renderers/albedo/albedo.cpp"

	"src/ui/editor.cpp"
	"src/ui/layout_editor.cpp"
	"src/ui/imgui_overlay.cpp"

	"src/ui/ui.cpp"
	"src/ui/window.cpp"
	"src/ui/widget.cpp"

	"src/ui/windows/debug_window.cpp"
	"src/ui/windows/imgui_style_editor_window.cpp"
	"src/ui/windows/rendered_view_window.cpp"
	"src/ui/windows/scene_editor_window.cpp"

	"src/ui/widgets/light_widget.cpp"
	"src/ui/widgets/material_widget.cpp"
	"src/ui/widgets/model_widget.cpp"
	"src/ui/widgets/node_widget.cpp"
	"src/ui/widgets/scene_graph_widget.cpp"
	"src/ui/widgets/world_widget.cpp"
	"src/ui/widgets/model_import_dialog.cpp"
	)

target_include_directories(bunsen PRIVATE "src")

target_link_libraries(bunsen PRIVATE 
	stdc++
	m
	dl
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
	message("GL_DEBUG is enabled!")
	target_compile_definitions(bunsen PRIVATE GL_DEBUG)
endif()

if (BUNSEN_DEBUG)
	message("Enabling debug features!")
	target_compile_definitions(bunsen PRIVATE BUNSEN_DEBUG)
endif()

add_custom_target(
	symlink_resources ALL
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	"${PROJECT_SOURCE_DIR}/resources" "${PROJECT_BINARY_DIR}/resources"
	)