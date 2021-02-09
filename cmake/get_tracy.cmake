cmake_minimum_required(VERSION 3.5)
include(FetchContent)

FetchContent_Populate(tracy
	GIT_REPOSITORY "https://github.com/wolfpld/tracy.git"
	GIT_TAG "v0.7.5"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tracy/tracy"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tracy/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/tracy/subbuild"
)

add_library(tracy "${tracy_SOURCE_DIR}/TracyClient.cpp")
target_include_directories(tracy PUBLIC "${tracy_SOURCE_DIR}")
