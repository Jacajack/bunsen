#include "ui.hpp"

void br::draw_ui(const br::borealis_state &main_state)
{
	ImGui::Begin("Scene manager");

	if (ImGui::ListBoxHeader("Objects"))
	{
		

		ImGui::Selectable("Default cube", false);
		ImGui::Selectable("Floor", false);
		ImGui::ListBoxFooter();
	}

	ImGui::End();



	// ImGui::Begin("Object properties");
	// if (ImGui::ListBoxHeader("Materials"))
	// {
	// 	ImGui::Selectable("red", false);
	// 	ImGui::Selectable("green", false);
	// 	ImGui::ListBoxFooter();
	// }
	// ImGui::End();



	// ImGui::Begin("Material properties");
	
	// ImGui::End();
}