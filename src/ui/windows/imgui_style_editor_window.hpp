#pragma once
#include "ui/window.hpp"

namespace bu::ui {

class imgui_style_editor_window : public window
{
public:
	imgui_style_editor_window() :
		window("ImGui Style Editor")
	{}

	void draw() override;
};

}