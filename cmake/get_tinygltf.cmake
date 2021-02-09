cmake_minimum_required(VERSION 3.5)
include(FetchContent)

FetchContent_Populate(tinygltf
	GIT_REPOSITORY "https://github.com/syoyo/tinygltf.git"
	GIT_TAG "v2.5.0"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tinygltf/tinygltf"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/tinygltf/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/tinygltf/subbuild"
)

add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE "${tinygltf_SOURCE_DIR}/../")
