project(borealis)
cmake_minimum_required(VERSION 3.3)

set(OpenGL_GL_PREFERENCE GLVND)

include(cmake/get_imgui.cmake)
include(cmake/get_tinygltf.cmake)
include(cmake/get_tracy.cmake)

find_package(glm 0.9.9 REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL 4.2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ASSIMP REQUIRED)

add_executable(borealis 
	"src/borealis.cpp"
	"src/ui.cpp"
	)

target_link_libraries(borealis PRIVATE 
	glm
	glfw
	GLEW
	OpenGL::GL
	nlohmann_json::nlohmann_json
	${ASSIMP_LIBRARIES}
	imgui_glfw
	# tinygltf
	# tracy
	)
	
file(COPY "${PROJECT_SOURCE_DIR}/resources" DESTINATION "${PROJECT_BINARY_DIR}")
