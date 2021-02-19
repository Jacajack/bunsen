#include "mesh_control.hpp"
#include <set>
#include <imgui.h>

/**
	\brief Displays mesh data info (vertex count, etc.)
*/
void bu::ui::mesh_data_info(std::shared_ptr<bu::mesh_data> mesh_data)
{
	// Table with mesh info
	ImGui::Text("Basic mesh info:");
	if (ImGui::BeginTable("Mesh info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		ImGui::Text("Vertices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->positions.size());

		ImGui::TableNextColumn();
		ImGui::Text("Normals");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->normals.size());

		ImGui::TableNextColumn();
		ImGui::Text("UVs");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->uvs.size());

		ImGui::TableNextColumn();
		ImGui::Text("Indices");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->indices.size());

		ImGui::TableNextColumn();
		ImGui::Text("Triangles");
		ImGui::TableNextColumn();
		ImGui::Text("%lu", mesh_data->indices.size() / 3);

		ImGui::EndTable();
	}
}

void bu::ui::mesh_menu(const bu::scene &scene, std::set<std::shared_ptr<bu::scene_node>> &selection)
{
	std::set<bu::mesh*> meshes;

	// Pointers to available mesh data and materials
	std::set<std::shared_ptr<bu::mesh_data>> mesh_data_ptrs;
	std::set<std::shared_ptr<bu::material_data>> material_ptrs;

	auto extract_meshes = [&meshes, &mesh_data_ptrs, &material_ptrs](bu::scene_node &n)
	{
		auto extract_meshes_impl = [&meshes,  &mesh_data_ptrs, &material_ptrs](bu::scene_node &n, auto &ref)->void
		{
			if (auto mn = dynamic_cast<bu::mesh_node*>(&n))
				for (auto &mesh : mn->meshes)
				{
					meshes.insert(&mesh);
					if (mesh.data) mesh_data_ptrs.insert(mesh.data);
					if (mesh.mat) material_ptrs.insert(mesh.mat);
				}
			
			for (auto &c : n.get_children())
				ref(*c, ref);
		};

		return extract_meshes_impl(n, extract_meshes_impl);
	};

	// Get all unique nodes and extract meshes from them
	for (auto &node : selection)
		extract_meshes(*node);

	static bu::mesh *selected_mesh; //TODO remove static
	bool selected_mesh_valid = false;

	// Show list box with meshes if there's more than one
	if (meshes.size() > 1)
	{
		ImGui::ListBoxHeader("Meshes");
		for (auto &mesh_ptr : meshes)
		{
			std::string text = mesh_ptr->data->name;

			bool selected = mesh_ptr == selected_mesh;
			selected_mesh_valid |= selected;
			if (ImGui::Selectable(text.c_str(), selected))
				selected_mesh = mesh_ptr;
		}
		ImGui::ListBoxFooter();
		ImGui::Separator();
	}
	else if (meshes.size() == 1)
	{
		selected_mesh = *meshes.begin();
		selected_mesh_valid = true;
	}

	if (selected_mesh_valid)
		bu::ui::mesh_data_info(selected_mesh->data);

}