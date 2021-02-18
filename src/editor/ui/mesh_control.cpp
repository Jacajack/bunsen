#include "mesh_control.hpp"
#include <set>
#include <imgui.h>

/**
	\brief Displays mesh info (vertex count, etc.) and list of all meshes in the selected nodes
*/
void bu::ui::mesh_info(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection)
{
	std::set<std::shared_ptr<bu::scene_node>> nodes;
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
	for (auto &wp : selection)
		if (auto node = wp.lock())
		{
			nodes.insert(node);
			extract_meshes(*node);
		}


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

	// Get mesh data an material pointers
	if (!selected_mesh_valid) return;
	auto mesh_data = selected_mesh->data;
	auto material_data = selected_mesh->mat;

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