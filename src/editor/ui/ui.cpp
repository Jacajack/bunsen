#include "ui.hpp"
#include "../../scene.hpp"
#include "../../log.hpp"
#include "../../assimp_loader.hpp"
#include <stack>
#include <queue>
#include <ImGuiFileDialog.h>

#if 0
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
#endif

void bu::imgui_cherry_theme()
{
    // cherry colors, 3 intensities
    #define HI(v)   ImVec4(0.502f, 0.075f, 0.256f, v)
    #define MED(v)  ImVec4(0.455f, 0.198f, 0.301f, v)
    #define LOW(v)  ImVec4(0.232f, 0.201f, 0.271f, v)
    // backgrounds (@todo: complete with BG_MED, BG_LOW)
    #define BG(v)   ImVec4(0.200f, 0.220f, 0.270f, v)
    // text
    #define TEXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

    auto &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = TEXT(0.78f);
    style.Colors[ImGuiCol_TextDisabled]          = TEXT(0.28f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]         = BG( 0.58f);
    style.Colors[ImGuiCol_PopupBg]               = BG( 0.9f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = BG( 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = MED( 0.78f);
    style.Colors[ImGuiCol_FrameBgActive]         = MED( 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = LOW( 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = HI( 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = BG( 0.75f);
    style.Colors[ImGuiCol_MenuBarBg]             = BG( 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg]           = BG( 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = MED( 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = MED( 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered]         = MED( 0.86f);
    style.Colors[ImGuiCol_ButtonActive]          = MED( 1.00f);
    style.Colors[ImGuiCol_Header]                = MED( 0.76f);
    style.Colors[ImGuiCol_HeaderHovered]         = MED( 0.86f);
    style.Colors[ImGuiCol_HeaderActive]          = HI( 1.00f);
    // style.Colors[ImGuiCol_Column]                = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    // style.Colors[ImGuiCol_ColumnHovered]         = MED( 0.78f);
    // style.Colors[ImGuiCol_ColumnActive]          = MED( 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = MED( 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive]      = MED( 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = MED( 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = TEXT(0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = MED( 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = MED( 0.43f);
    // [...]
    style.Colors[ImGuiCol_ModalWindowDimBg]  = BG( 0.73f);

    style.WindowPadding            = ImVec2(6, 4);
    style.WindowRounding           = 0.0f;
    style.FramePadding             = ImVec2(5, 2);
    style.FrameRounding            = 3.0f;
    style.ItemSpacing              = ImVec2(7, 1);
    style.ItemInnerSpacing         = ImVec2(1, 1);
    style.TouchExtraPadding        = ImVec2(0, 0);
    style.IndentSpacing            = 6.0f;
    style.ScrollbarSize            = 12.0f;
    style.ScrollbarRounding        = 16.0f;
    style.GrabMinSize              = 20.0f;
    style.GrabRounding             = 2.0f;

    style.WindowTitleAlign.x = 0.50f;

    style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
}