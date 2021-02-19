#include "scene_control.hpp"
#include <set>
#include <algorithm>
#include <string>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_icon_font_headers/IconsFontAwesome5.h>
#include <glm/gtc/quaternion.hpp>

/**
	\brief Responsible for displaying the scene tree
	\todo Dragging for regrouping the nodes
*/
void bu::ui::scene_graph(const bu::scene &scene, bu::scene_selection &selection)
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("Scene Tree", ImVec2(ImGui::GetWindowContentRegionWidth(), 200), false, window_flags);

	std::stack<std::shared_ptr<bu::scene_node>> node_stack;
	node_stack.push(scene.root_node);
	while (node_stack.size())
	{
		auto node_ptr = node_stack.top();
		node_stack.pop();		

		// Going up
		if (!node_ptr)
		{
			ImGui::TreePop();
			continue;
		}

		// Default node flags
		ImGuiTreeNodeFlags node_flags = 
			ImGuiTreeNodeFlags_OpenOnArrow 
			| ImGuiTreeNodeFlags_OpenOnDoubleClick 
			| ImGuiTreeNodeFlags_SpanFullWidth 
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		// Highlight if selected
		if (selection.contains(node_ptr))
			node_flags |= ImGuiTreeNodeFlags_Selected;

		// Assume the node is open (because of temporary transform nodes)
		bool node_open = true;
		bool display_node = true;

		// Do not display transform nodes on the list
		if (dynamic_cast<bu::transform_node*>(node_ptr.get()))
			display_node = false;

		// Check if the node is visible
		bool visible = node_ptr->is_visible();

		// Dim if invisible
		if (!visible)
		{
			auto col = ImGui::GetStyleColorVec4(ImGuiCol_Text);
			col.w *= 0.5;
			ImGui::PushStyleColor(ImGuiCol_Text, col);
		}

		if (display_node)
		{
			std::string name_tags;
			if (dynamic_cast<bu::mesh_node*>(node_ptr.get()))
				name_tags += ICON_FA_DRAW_POLYGON " ";

			if (dynamic_cast<bu::light_node*>(node_ptr.get()))
				name_tags += ICON_FA_LIGHTBULB " ";

			if (dynamic_cast<bu::camera_node*>(node_ptr.get()))
				name_tags += ICON_FA_VIDEO " ";

			node_open = ImGui::TreeNodeEx(node_ptr.get(), node_flags, "%s%s", name_tags.c_str(), node_ptr->get_name().c_str());
			
			// On click - select or deselect
			// Ctrl + click - toggle
			// If multiple items are selected, and control is not held - select
			if (ImGui::IsItemClicked())
			{
				bool append = ImGui::GetIO().KeyCtrl;
				selection.click(node_ptr, append);
			}
		}

		// If open, draw children
		if (node_open)
		{
			// Push end marker and children on stack
			if (display_node)
				node_stack.push(nullptr);

			for (auto &c : node_ptr->get_children())
				node_stack.push(c);

			// List meshes
			if (auto mesh_node_ptr = dynamic_cast<bu::mesh_node*>(node_ptr.get()))
			{
				ImGuiTreeNodeFlags leaf_flags = 
					ImGuiTreeNodeFlags_SpanAvailWidth 
					| ImGuiTreeNodeFlags_SpanFullWidth 
					| ImGuiTreeNodeFlags_Leaf 
					| ImGuiTreeNodeFlags_NoTreePushOnOpen;

				for (const auto &mesh : mesh_node_ptr->meshes)
					if (mesh.data)
						ImGui::TreeNodeEx(&mesh, leaf_flags, ICON_FA_DRAW_POLYGON " %s", mesh.data->name.c_str());

				for (const auto &mesh : mesh_node_ptr->meshes)
					if (mesh.mat)
						ImGui::TreeNodeEx(&mesh, leaf_flags, ICON_FA_GEM " %s", mesh.mat->name.c_str());

			}
		}

		// Pop dim
		if (!visible)
			ImGui::PopStyleColor();
	}

	ImGui::EndChild();
}

/**
	\brief Node controls (grouping and deleting)
*/
void bu::ui::node_controls(const bu::scene &scene, bu::scene_selection &selection)
{
	bool selection_empty = selection.empty();
	bool multiple_selection = selection.size() > 1;
	bool contains_root_node = false;
	bool have_common_parent = true;
	std::shared_ptr<bu::scene_node> common_parent;

	for (auto &node : selection.get_nodes())
	{
		if (node == scene.root_node) contains_root_node = true;
		if (!common_parent)
			common_parent = node->get_parent().lock();
		else if (common_parent != node->get_parent().lock())
			have_common_parent = false;
	}

	bool disable_delete = selection_empty || contains_root_node;
	bool disable_dissolve = selection_empty || contains_root_node;
	bool disable_group = selection_empty || contains_root_node || !have_common_parent || !common_parent;
	bool disable_duplicate = selection_empty || contains_root_node;
	bool disable_add = selection_empty || multiple_selection;
	bool clicked_delete = false;
	bool clicked_dissolve = false;
	bool clicked_group = false;
	bool clicked_duplicate = false;
	bool clicked_add_light = false;
	bool clicked_import_mesh = false;

	auto push_disable = [](bool disable)
	{
		if (!disable) return;
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	};

	auto pop_disable = [](bool disable)
	{
		if (!disable) return;
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	};

	if (ImGui::BeginTable("split", 2))
	{
		auto button_size = [](){return ImVec2(ImGui::GetContentRegionAvailWidth(), 0);};

		ImGui::TableNextColumn();
		push_disable(disable_delete);
		clicked_delete = ImGui::Button("Delete", button_size());
		pop_disable(disable_delete);

		ImGui::TableNextColumn();	
		push_disable(disable_dissolve);
		clicked_dissolve = ImGui::Button("Dissolve", button_size());
		pop_disable(disable_dissolve);

		ImGui::TableNextColumn();
		push_disable(disable_group);
		clicked_group = ImGui::Button("Group", button_size());
		pop_disable(disable_group);

		ImGui::TableNextColumn();
		push_disable(disable_duplicate);
		clicked_duplicate = ImGui::Button("Duplicate", button_size());
		pop_disable(disable_duplicate);

		ImGui::TableNextColumn();
		push_disable(true);
		ImGui::Button("Add camera", button_size());
		pop_disable(true);

		ImGui::TableNextColumn();
		push_disable(disable_add);
		clicked_add_light = ImGui::Button("Add light", button_size());
		pop_disable(disable_add);

		ImGui::TableNextColumn();
		push_disable(true);
		ImGui::Button("Add mesh", button_size());
		pop_disable(true);

		ImGui::TableNextColumn();
		push_disable(disable_add);
		clicked_import_mesh = ImGui::Button("Import mesh", button_size());
		pop_disable(disable_add);

		ImGui::EndTable();
	}

	// Delete node
	if (clicked_delete)
	{
		for (auto &n : selection.get_nodes())
			n->remove_from_parent();
		selection.clear();
	}

	// Dissolve node
	if (clicked_dissolve)
	{
		for (auto &n : selection.get_nodes())
			n->dissolve();
		selection.clear();
	}

	// Group nodes
	if (clicked_group)
	{
		auto group_node = std::make_shared<bu::scene_node>();
		group_node->set_name("Group Node");
		for (auto &n : selection.get_nodes())
			n->set_parent(group_node);
		group_node->set_parent(common_parent);
	}

	// Duplicate nodes
	if (clicked_duplicate)
	{
		for (auto &n : selection.get_nodes())
		{
			auto p = n->get_parent().lock();
			auto dup = n->clone(p);
			dup->set_name(dup->get_name() + " (dup)");
			selection.clear();
			selection.select(dup);
			// TODO trigger grab
		}
	}

	// Import mesh
	if (clicked_import_mesh)
	{
		// TODO
		// auto parent = (*selection.begin()).lock();

	}

	// Add light
	if (clicked_add_light)
	{
		auto parent = scene.root_node;
		auto light = std::make_shared<bu::light_node>();
		light->set_name("New Light");
		parent->add_child(light);
	}
}

/**
	\brief Property editor for nodes (transforms, names)
	\todo Make transform UI affect the objects
*/
void bu::ui::node_properties(const std::shared_ptr<bu::scene_node> &node)
{
	bool visible = node->is_visible();
	bool local_transform = node->get_transform_origin() == bu::scene_node::node_transform_origin::PARENT;
	
	// Name buffer
	const auto name_max_len = 32u;
	std::string name = node->get_name();
	std::vector<char> buf(std::max(name_max_len, static_cast<unsigned int>(name.size()) + 1));
	std::copy(name.begin(), name.end(), buf.begin());

	// Decompose transform
	glm::vec3 translate;
	glm::mat4 mat = node->get_transform();
	translate = mat[3];
	mat[3] = glm::vec4{0.f};
	glm::vec3 scale{glm::length(mat[0]), glm::length(mat[1]), glm::length(mat[2])};
	mat[0] /= scale.x;
	mat[1] /= scale.y;
	mat[2] /= scale.z;
	auto quat = glm::quat_cast(mat);

	// Reassemble matrix
	// quat = glm::normalize(quat);
	// mat = glm::translate(glm::scale(glm::mat4_cast(quat), scale), translate);

	// Name 
	ImGui::InputText("Name", buf.data(), name_max_len);
	ImGui::Dummy(ImVec2(0.f, 5.f));
	
	// Transforms (currently disabled)
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	ImGui::DragFloat3("Translation", &translate[0], 0.1f, 0, 0);
	ImGui::DragFloat4("Rotation", &quat[0], 0.1f, 0, 0);
	ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0, 0);
	ImGui::PopItemFlag();
	ImGui::PopStyleVar();
	ImGui::Dummy(ImVec2(0.f, 5.f));

	// Other attributes
	bool changed_local_transform = ImGui::Checkbox("Local transform", &local_transform);
	bool changed_visible = ImGui::Checkbox("Visible", &visible);
	
	// Write back to the node
	if (changed_visible) node->set_visible(visible);
	if (changed_local_transform) node->set_transform_origin(local_transform ? bu::scene_node::node_transform_origin::PARENT : bu::scene_node::node_transform_origin::WORLD);
	// node->set_transform(mat);

	// Do not pass entire buffer to std::string{} because it contains terminating NULs
	auto len = std::strlen(buf.data());
	node->set_name(std::string{buf.begin(), buf.begin() + len});
}

void bu::ui::node_menu(const bu::scene &scene, bu::scene_selection &selection)
{
	std::shared_ptr<bu::scene_node> node;
	if (selection.has_primary()) node = selection.get_primary();
	if (selection.size() != 1 || !node)
	{
		ImGui::TextWrapped("Nothing to see here, buddy...");
		return;
	}
	
	bu::ui::node_properties(node);
}