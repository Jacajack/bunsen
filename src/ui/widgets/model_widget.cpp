#include "model_widget.hpp"
#include <imgui.h>

using bu::ui::model_widget;

/**
	\brief Displays mesh data info (vertex count, etc.)
*/
void model_widget::draw_mesh_info(const std::shared_ptr<bu::mesh> &mesh)
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

void model_widget::draw(const std::vector<std::shared_ptr<bu::model>> &models)
{
	auto selected_model = m_selected_model.lock();
	bool selected_model_valid = false;

	for (auto model : models)
		if (model == selected_model)
			selected_model_valid = true;

	if (!selected_model_valid)
	{
		m_selected_model.reset();
		m_selected_pair = nullptr;
	}

	bool selected_pair_valid = false;

	ImGui::ListBoxHeader("Meshes");
	for (auto &model : models)
	{
		for (auto &mesh_mat : model->meshes)
		{
			std::string text = mesh_mat.mesh->name;
			bool selected = &mesh_mat == m_selected_pair;
			selected_pair_valid |= selected;

			if (ImGui::Selectable(text.c_str(), selected))
			{
				m_selected_model = model;
				m_selected_pair = &mesh_mat;
				selected_pair_valid = true;
			}
		}
	}
	ImGui::ListBoxFooter();

	ImGui::Separator();

	if (!selected_pair_valid)
	{
		m_selected_pair = nullptr;
		m_selected_model.reset();
	}

	if (selected_pair_valid)
	{
		if (ImGui::BeginCombo("Material", "Material"))
		{
			ImGui::Selectable("It's just a layout test");
			ImGui::EndCombo();
		}

		draw_mesh_info(m_selected_pair->mesh);
	}
}
