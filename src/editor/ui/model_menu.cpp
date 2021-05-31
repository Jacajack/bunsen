#include "model_menu.hpp"
#include <set>
#include <string>
#include <imgui.h>
#include "../../scene.hpp"

/**
	\brief Displays mesh data info (vertex count, etc.)
*/
void bu::ui::mesh_info(const std::shared_ptr<bu::mesh> &mesh)
{
	// Table with mesh info
	ImGui::Text("Basic mesh info:");
	if (ImGui::BeginTable("Mesh info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		ImGui::Text("Vertices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh->vertices.size());

		ImGui::TableNextColumn();
		ImGui::Text("Normals");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh->normals.size());

		ImGui::TableNextColumn();
		ImGui::Text("UVs");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh->uvs.size());

		ImGui::TableNextColumn();
		ImGui::Text("Indices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh->indices.size());

		ImGui::TableNextColumn();
		ImGui::Text("Triangles");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh->indices.size() / 3);

		ImGui::EndTable();
	}
}

void bu::ui::model_menu(bu::scene_selection &selection)
{
	auto primary = selection.get_primary();
	if (!primary) return;
	auto model_node = std::dynamic_pointer_cast<bu::model_node>(primary);
	if (!model_node) return;
	auto model = model_node->model;

	static bu::model::mesh_material_pair *selected_mesh_mat; //TODO remove static
	bool selected_mesh_valid = false;

	ImGui::ListBoxHeader("Meshes");
	for (auto &mesh_mat : model->meshes)
	{
		std::string text = mesh_mat.mesh->name;
		bool selected = &mesh_mat == selected_mesh_mat;
		selected_mesh_valid |= selected;
		if (ImGui::Selectable(text.c_str(), selected))
		{
			selected_mesh_mat = &mesh_mat;
			selected_mesh_valid = true;
		}
	}
	ImGui::ListBoxFooter();

	ImGui::Separator();

	if (!selected_mesh_valid && model->meshes.size())
	{
		selected_mesh_mat = &model->meshes[0];
		selected_mesh_valid = true;
	}

	if (selected_mesh_valid)
	{
		if (ImGui::BeginCombo("Material", "Material"))
		{
			ImGui::Selectable("It's just a layout test");
			ImGui::EndCombo();
		}

		bu::ui::mesh_info(selected_mesh_mat->mesh);
	}
}