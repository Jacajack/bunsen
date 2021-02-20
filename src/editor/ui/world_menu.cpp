#include "world_menu.hpp"
#include <imgui.h>

void bu::ui::world_menu(bu::world &world)
{
	ImGui::Text("World properties");
	ImGui::Indent();

	if (auto w = dynamic_cast<solid_world*>(&world))
	{
		ImGui::ColorEdit3("World color", &w->color[0]);
	}

	ImGui::Unindent();
}