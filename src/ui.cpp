#include "ui.hpp"
#include "scene.hpp"
#include "log.hpp"
#include "assimp_loader.hpp"
#include <stack>
#include <queue>
#include <ImGuiFileDialog.h>

void bu::draw_ui(ui_state &ui, const bu::bunsen_state &main_state)
{
	auto &scene = *main_state.current_scene;

	ImGui::Begin("Scene manager");

	static bool is_node = false;
	static bool is_mesh = false;
	static void *selection = nullptr;
	bool selection_valid = false;
	
	if (ImGui::CollapsingHeader("Scene tree"))
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::BeginChild("Scene Tree", ImVec2(ImGui::GetWindowContentRegionWidth(), 200), false, window_flags);

		std::stack<scene_node*> node_stack;
		node_stack.push(&scene.root_node);
		while (node_stack.size())
		{
			scene_node *node_ptr = node_stack.top();
			node_stack.pop();		

			if (node_ptr)
			{
				auto &node = *node_ptr;
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
					| ImGuiTreeNodeFlags_OpenOnDoubleClick 
					| ImGuiTreeNodeFlags_SpanFullWidth 
					| ImGuiTreeNodeFlags_SpanAvailWidth;
				if (is_node && selection == node_ptr)
				{
					node_flags |= ImGuiTreeNodeFlags_Selected;
					selection_valid = true;
				}
				bool node_open = ImGui::TreeNodeEx(node_ptr, node_flags, "%s", node.name.c_str());
				if (ImGui::IsItemClicked())
				{
					if (selection != node_ptr)
					{
						is_mesh = false;
						is_node = true;
						selection = node_ptr;
					}
					else
					{
						selection = nullptr;
					}
				}	

				if (node_open)
				{
					node_stack.push(nullptr);
					for (auto &c : node.children)
						node_stack.push(&c);

					for (auto &mesh : node.meshes)
					{
						ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow 
							| ImGuiTreeNodeFlags_OpenOnDoubleClick 
							| ImGuiTreeNodeFlags_SpanAvailWidth 
							| ImGuiTreeNodeFlags_SpanFullWidth 
							| ImGuiTreeNodeFlags_Leaf 
							| ImGuiTreeNodeFlags_NoTreePushOnOpen;
						if (is_mesh && selection == &mesh)
						{
							node_flags |= ImGuiTreeNodeFlags_Selected;
							selection_valid = true;
						}
						ImGui::TreeNodeEx(&mesh, node_flags, "[M] %s", mesh.data->name.c_str());
						if (ImGui::IsItemClicked())
						{
							if (selection != &mesh)
							{
								is_mesh = true;
								is_node = false;
								selection = &mesh;
							}
							else
							{
								selection = nullptr;
							}
						}	
					}
				}
			}
			else
				ImGui::TreePop();
		}

		ImGui::EndChild();
	}

	

	ImGui::Separator();

	if (selection_valid && selection)
	{
		if (is_node)
		{
			auto &node = *reinterpret_cast<bu::scene_node*>(selection);
			ImGui::TextWrapped("Selected node '%s' with %d children", node.name.c_str(), node.children.size());
			ImGui::SliderFloat3("Translate", &node.transform[3][0], -10, 10);
			ui.selected_node = &node;
		}

		if (is_mesh)
		{
			auto &mesh = *reinterpret_cast<bu::mesh*>(selection);
			if (mesh.data)
			{
				ImGui::TextWrapped("Mesh data named '%s' consists of %d vertices...", mesh.data->name.c_str(), mesh.data->positions.size());
			}
			ui.selected_node = nullptr;
		}
	}
	else
	{
		ui.selected_node = nullptr;
	}

	ImGui::Separator();


	// if (ImGui::TreeNode("aaaa"))
	// {
	// 	if (ImGui::TreeNode("aaaasdsaa"))
	// 	{
	// 		ImGui::TreePop();
	// 	}
	// 	ImGui::TreePop();
	// }

	// if (ImGui::ListBoxHeader("Objects"))
	// {
		

	// 	ImGui::Selectable("Default cube", false);
	// 	ImGui::Selectable("Floor", false);
	// 	ImGui::ListBoxFooter();
	// }

	if (ImGui::Button("Open File Dialog"))
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".obj,.dae,.gltf,.glb", ".");

	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
			try
			{
				scene.root_node.children.emplace_back(bu::load_mesh_from_file(path));
			}
			catch (const std::exception &ex)
			{
				LOG_ERROR << "failed to load model '" << path << "' - " << ex.what();
			}
		}

		ImGuiFileDialog::Instance()->Close();
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