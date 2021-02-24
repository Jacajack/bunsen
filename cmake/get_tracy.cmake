cmake_minimum_required(VERSION 3.5)
include(FetchContent)

option(TRACY_ENABLE "Should Tracy be enabled?" OFF)

FetchContent_Populate(tracy
	GIT_REPOSITORY "https://github.com/wolfpld/tracy.git"
	GIT_TAG "v0.7.6"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tracy/tracy"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tracy/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/tracy/subbuild"
)

add_library(tracy STATIC "${tracy_SOURCE_DIR}/TracyClient.cpp")
target_link_libraries(tracy PUBLIC "dl")
target_include_directories(tracy PUBLIC "${tracy_SOURCE_DIR}/..")
target_compile_definitions(tracy PUBLIC "TRACY_ENABLE")

if (TRACY_ENABLE)
	message("Tracy is enabled!")
endif()