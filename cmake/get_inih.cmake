cmake_minimum_required(VERSION 3.5)
include(FetchContent)

FetchContent_Populate(inih
	GIT_REPOSITORY "https://github.com/benhoyt/inih.git"
	GIT_TAG "r53"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/inih/inih"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/inih/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/inih/subbuild"
)

add_library(inih 
	"${inih_SOURCE_DIR}/cpp/INIReader.cpp"
	"${inih_SOURCE_DIR}/ini.c"
	)
target_include_directories(inih PUBLIC "${inih_SOURCE_DIR}/../")
