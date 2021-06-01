#include "scene_editor_window.hpp"
#include <set>
#include <stack>
#include <algorithm>
#include <string>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>

#include "ui/editor.hpp"
#include "ui/icons.hpp"
#include "scene.hpp"
#include "scene_selection.hpp"

using bu::ui::scene_editor_window;

/**
	\brief Node controls (grouping and deleting)
*/
void scene_editor_window::draw_buttons(const bu::scene &scene, bu::scene_selection &selection)
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
		m_open_model_import_dialog = true;

	// Add light
	if (clicked_add_light)
	{
		auto parent = scene.root_node;
		auto light_node = std::make_shared<bu::light_node>();
		light_node->set_name("New Point Light");
		auto light = std::make_shared<bu::point_light>();
		light_node->light = light;
		parent->add_child(light_node);
	}
}

void scene_editor_window::draw()
{
	m_open_model_import_dialog = false;
	auto &ed = m_editor;

	// Titlebar menu
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Scene"))
		{
			ImGui::MenuItem("Load", "l", nullptr);
			ImGui::MenuItem("Save", "s", nullptr);
		
			if (ImGui::MenuItem("Import model", "i", nullptr))
				m_open_model_import_dialog = true;

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	
	static int tab_select = 0;

	m_scene_graph_widget.draw(*ed.scene, ed.scene->selection);
	ImGui::Separator();
	draw_buttons(*ed.scene, ed.scene->selection);		

	ImGui::Separator();

	// Check what type of node is selected so proper tabs can be dimmed out
	auto primary = ed.scene->selection.get_primary();
	bool primary_valid = bool(primary);
	bool is_model_node = dynamic_cast<bu::model_node*>(primary.get());
	bool is_light_node = dynamic_cast<bu::light_node*>(primary.get());
	bool is_camera_node = dynamic_cast<bu::camera_node*>(primary.get());

	std::vector<std::shared_ptr<bu::light>> lights;
	std::vector<std::shared_ptr<bu::model>> models;
	std::vector<std::shared_ptr<bu::material_data>> materials;
	std::vector<std::shared_ptr<bu::scene_node>> selected_nodes;

	for (auto node : ed.scene->selection.get_nodes())
	{
		if (auto ptr = std::dynamic_pointer_cast<bu::light_node>(node))
			lights.push_back(ptr->light);

		if (auto ptr = std::dynamic_pointer_cast<bu::model_node>(node))
			models.push_back(ptr->model);

		selected_nodes.push_back(node);
	}

	for (auto model : models)
		std::copy(model->materials.begin(), model->materials.end(), std::back_inserter(materials));

	// Tabs
	if (ImGui::BeginTable("tab buttons", 7))
	{
		auto tab_button = [](const char *text, int id, bool enable)
		{
			auto tab_color = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
			auto active_color = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
			auto color = tab_select == id ? active_color : tab_color;
			ImVec2 button_size(ImGui::GetContentRegionAvail().x, 25);

			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active_color);
			ImGui::PushStyleColor(ImGuiCol_Button, color);

			if (!enable)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
				if (tab_select == id)
					tab_select = 0;
			}
			
			if (ImGui::Button(text, button_size) && enable) tab_select = id;

			if (!enable)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			ImGui::PopStyleColor(2);
		};

		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_RENDERER, 0, true);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_WORLD, 6, true);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_MODEL, 1, primary_valid);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_MESH, 2, is_model_node);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_MATERIAL, 3, is_model_node);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_LIGHT, 4, is_light_node);
		ImGui::TableNextColumn(); tab_button(BUNSEN_ICON_CAMERA, 5, is_camera_node);
	
		ImGui::EndTable();
	}

	ImGui::Separator();

	// Child window for containing the external menus
	if (ImGui::BeginChild("tab space"))
	{
		switch (tab_select)
		{
			case 0:
				ImGui::Text("Renderer settings....");
				break;

			case 1:
				if (primary)
					m_node_widget.draw(primary);
				break;

			case 2:
				m_model_widget.draw(models);
				break;

			case 3:
				m_material_widget.draw(materials);
				break;

			case 4:
				if (!lights.empty())
					m_light_widget.draw(lights.front());
				break;

			case 5:
				ImGui::Text("Camera properties...");
				break;

			case 6:
				if (ed.scene->world)
					m_world_widget.draw(*ed.scene->world);
				break;
		}
	}

	ImGui::EndChild();

	m_model_import_dialog.draw(ed.scene, m_open_model_import_dialog);
}