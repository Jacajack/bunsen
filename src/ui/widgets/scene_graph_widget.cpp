#include "scene_graph_widget.hpp"

#include <stack>
#include <imgui.h>
#include <imgui_icon_font_headers/IconsFontAwesome5.h>

#include "scene.hpp"
#include "scene_selection.hpp"

using bu::ui::scene_graph_widget;

/**
	\brief Responsible for displaying the scene tree
	\todo Dragging for regrouping the nodes
*/
void scene_graph_widget::draw(const bu::scene &scene, bu::scene_selection &selection)
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

		// Slightly emphasize primary selection
		bool color_emphasis = false;
		if (selection.is_primary(node_ptr))
		{
			color_emphasis = true;
			ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Header);
			col.x *= 1.3;
			col.y *= 1.3;
			col.z *= 1.3;
			ImGui::PushStyleColor(ImGuiCol_Header, col);

			col = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
			col.x *= 1.3;
			col.y *= 1.3;
			col.z *= 1.3;
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, col);
		}

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
			if (dynamic_cast<bu::model_node*>(node_ptr.get()))
				name_tags += ICON_FA_CUBE " ";

			if (dynamic_cast<bu::light_node*>(node_ptr.get()))
				name_tags += ICON_FA_LIGHTBULB " ";

			if (dynamic_cast<bu::camera_node*>(node_ptr.get()))
				name_tags += ICON_FA_VIDEO " ";

			node_open = ImGui::TreeNodeEx(node_ptr.get(), node_flags, "%s%s", name_tags.c_str(), node_ptr->get_name().c_str());

			// Drag source for all nodes other than the root
			if (node_ptr != scene.root_node && ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("scene_node", node_ptr.get(), sizeof(bu::scene_node));
				ImGui::Text("Set parent for node '%s'", node_ptr->get_name().c_str());
				ImGui::EndDragDropSource();
			}

			// Accept dropped nodes other than itself and parents
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("scene_node"))
				{
					auto dropped_ptr = reinterpret_cast<bu::scene_node*>(payload->Data)->shared_from_this();
					if (dropped_ptr && dropped_ptr != node_ptr && !dropped_ptr->is_ancestor_of(node_ptr))
						dropped_ptr->set_parent(node_ptr);
				}
				ImGui::EndDragDropTarget();
			}
			
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
			if (auto model_node_ptr = dynamic_cast<bu::model_node*>(node_ptr.get()))
			{
				ImGuiTreeNodeFlags leaf_flags = 
					ImGuiTreeNodeFlags_SpanAvailWidth 
					| ImGuiTreeNodeFlags_SpanFullWidth 
					| ImGuiTreeNodeFlags_Leaf 
					| ImGuiTreeNodeFlags_NoTreePushOnOpen;

				// Leafs are bit dimmed when selected
				ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_Header);
				col.w *= 0.5;
				ImGui::PushStyleColor(ImGuiCol_Header, col);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, col);

				// Highlight if selected
				if (selection.contains(node_ptr))
					leaf_flags |= ImGuiTreeNodeFlags_Selected;

				bool clicked = false;

				if (model_node_ptr->model)
				{
					for (const auto &mesh : model_node_ptr->model->meshes)
					{
						if (mesh.mesh)
						{
							ImGui::TreeNodeEx(&mesh, leaf_flags, ICON_FA_DRAW_POLYGON " %s", mesh.mesh->name.c_str());
							clicked |= ImGui::IsItemClicked();
						}
					}

					for (const auto &mat : model_node_ptr->model->materials)
						if (mat)
						{
							ImGui::TreeNodeEx(mat.get(), leaf_flags, ICON_FA_GEM " %s", mat->name.c_str());
							clicked |= ImGui::IsItemClicked();
						}
				}

				// Clicking on leafs acts like clicking on the node
				if (clicked)
				{
					bool append = ImGui::GetIO().KeyCtrl;
					selection.click(node_ptr, append);
				}

				// Pop dimmed background colors
				ImGui::PopStyleColor(2);
			}
		}

		// Pop dim
		if (!visible)
			ImGui::PopStyleColor();

		// Pop emphasized color
		if (color_emphasis)
			ImGui::PopStyleColor(2);
	}

	ImGui::EndChild();
}
