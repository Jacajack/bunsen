include(FetchContent)

FetchContent_Populate(imgui_icon_font_headers
	GIT_REPOSITORY "https://github.com/juliettef/IconFontCppHeaders.git"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/imgui_icon_font_headers/imgui_icon_font_headers"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/imgui_icon_font_headers/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/imgui_icon_font_headers/subbuild"
)

add_library(imgui_icon_font_headers INTERFACE)
target_include_directories(imgui_icon_font_headers INTERFACE
	"${imgui_icon_font_headers_SOURCE_DIR}/.."
)
