#include "imgui_style_editor_window.hpp"
#include <imgui.h>
using bu::ui::imgui_style_editor_window;

void imgui_style_editor_window::draw()
{
	ImGui::ShowStyleEditor(&ImGui::GetStyle());
}