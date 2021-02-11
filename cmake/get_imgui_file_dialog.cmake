include(FetchContent)

FetchContent_Populate(imgui_file_dialog
	GIT_REPOSITORY "https://github.com/aiekick/ImGuiFileDialog.git"
	GIT_TAG "Lib_Only"
	SOURCE_DIR   "${CMAKE_CURRENT_BINARY_DIR}/imgui_file_dialog/imgui_file_dialog"
	BINARY_DIR   "${CMAKE_CURRENT_BINARY_DIR}/imgui_file_dialog/bin"
	SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/imgui_file_dialog/subbuild"
)

add_library(imgui_file_dialog STATIC
	"${imgui_file_dialog_SOURCE_DIR}/ImGuiFileDialog.cpp"
	)
target_link_libraries(imgui_file_dialog PUBLIC imgui_glfw)
target_include_directories(imgui_file_dialog PUBLIC
	"${imgui_file_dialog_SOURCE_DIR}"
	)
