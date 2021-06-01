#include "world_widget.hpp"
#include <imgui.h>
#include "world.hpp"

using bu::ui::world_widget;

void world_widget::draw(bu::world &world)
{
	ImGui::Text("World properties");
	ImGui::Indent();

	if (auto w = dynamic_cast<solid_world*>(&world))
	{
		ImGui::ColorEdit3("World color", &w->color[0]);
	}

	ImGui::Unindent();
}