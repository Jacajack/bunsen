#include "light_widget.hpp"
#include "light.hpp"
#include <imgui.h>

using bu::ui::light_widget;

void light_widget::draw(std::shared_ptr<bu::light> light)
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
