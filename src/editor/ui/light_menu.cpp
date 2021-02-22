#include "light_menu.hpp"
#include "../../scene.hpp"
#include <imgui.h>

void bu::ui::light_editor(const std::shared_ptr<bu::light> &light)
{
	if (auto l = std::dynamic_pointer_cast<bu::point_light>(light))
	{
		ImGui::Text("Point light properties");
		ImGui::Indent();

		ImGui::ColorEdit3("Color", &l->color[0]);
		ImGui::DragFloat("Power", &l->power, 0.01, 0, 100);
		ImGui::SliderFloat("Radius", &l->radius, 0, 5);

		ImGui::Unindent();
	}
}

void bu::ui::light_menu(bu::scene_selection &selection)
{
	auto primary = selection.get_primary();
	if (!primary) return;
	auto light_node = std::dynamic_pointer_cast<bu::light_node>(primary);
	if (!light_node) return;
	auto light = light_node->light;
	if (!light) return;

	bu::ui::light_editor(light);
}