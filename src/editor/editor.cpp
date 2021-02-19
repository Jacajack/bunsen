#include "editor.hpp"
#include <stack>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>

#include "../assimp_loader.hpp"
#include "../material.hpp"
#include "../log.hpp"
#include "ui/ui.hpp"
#include "ui/scene_control.hpp"
#include "ui/material_editor.hpp"
#include "ui/mesh_control.hpp"

#include <imgui_icon_font_headers/IconsFontAwesome5.h>

using bu::bunsen_editor;

/**
	\brief Displays/opens model import dialog window and imports models
*/
static void dialog_import_model(bunsen_editor &ed, bool open = false)
{
	static ImGuiFileDialog dialog;

	if (open && ed.scene)
	{
		static ImVec4 file_highlight(0.8, 0, 0.3, 0.9);
		dialog.SetExtentionInfos(".obj", file_highlight);
		dialog.SetExtentionInfos(".dae", file_highlight);
		dialog.SetExtentionInfos(".gltf", file_highlight);
		dialog.SetExtentionInfos(".glb", file_highlight);

		dialog.OpenDialog("import_model",
			"Import a model from file",
			"OBJ files (*.obj){.obj},"
			"Collada files (*.dae){.dae},"
			"GLTF (*.gltf *.glb){.gltf,.glb}",
			""
		);
	}

	if (dialog.Display("import_model"))
	{
		if (dialog.IsOk())
		{
			std::string path = dialog.GetFilePathName();
			if (ed.scene)
			{
				try
				{
					ed.scene->root_node->add_child(bu::load_mesh_from_file(path));
				}
				catch (const std::exception &ex)
				{
					LOG_ERROR << "Failed to load model '" << path << "' - " << ex.what();
				}
			}
			else
			{
				LOG_ERROR << "Cannot import models - there is no scene!";
			}
		}

		dialog.Close();
	}
}

/**
	\brief Displays a window with some debugging cheats
*/
static void window_debug_cheats(bunsen_editor &ed)
{
	ImGui::Begin("Evil debug cheats");

	if (ImGui::Button("Reload preview renderer"))
	{
		ed.preview.reset();
		try
		{
			ed.preview = std::make_unique<bu::preview_renderer>();
		}
		catch (const std::exception &ex)
		{
			LOG_ERROR << "Failed to reload preview mode! - " << ex.what();
		}
	}

	static bool show_style_editor = false;
	ImGui::Checkbox("Show ImGui style editor", &show_style_editor);
	if (show_style_editor)
	{
		ImGui::Begin("Style Editor");
		ImGui::ShowStyleEditor(&ImGui::GetStyle());
		ImGui::End();
	}

	static float col[3];
	if (ImGui::ColorEdit3("Color theme base", col))
	{
		bu::ui::load_theme(col[0], col[1], col[2]);
	}

	ImGui::End();
}

static void window_editor(bunsen_editor &ed)
{
	if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_MenuBar))
	{
		// Titlebar menu
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				ImGui::MenuItem("Load", "l", nullptr);
				ImGui::MenuItem("Save", "s", nullptr);
			
				if (ImGui::MenuItem("Import model", "i", nullptr))
					dialog_import_model(ed, true);

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
		
		static int tab_select = 0;
	
		if (ed.scene)
		{
			bu::ui::scene_graph(*ed.scene, ed.selected_nodes);
			ImGui::Separator();
			bu::ui::node_controls(*ed.scene, ed.selected_nodes);		
		}

		ImGui::Separator();

		if (ImGui::BeginTable("tab buttons", 6))
		{
			auto tab_button = [](const char *text, int id)
			{
				ImU32 tab_color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Tab));
				ImU32 active_color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
				ImVec2 button_size(ImGui::GetContentRegionAvail().x, 25);

				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active_color);

				if (tab_select == id)
					ImGui::PushStyleColor(ImGuiCol_Button, active_color);
				else
					ImGui::PushStyleColor(ImGuiCol_Button, tab_color);
				
				if (ImGui::Button(text, button_size)) tab_select = id;

				ImGui::PopStyleColor(2);
			};

			ImGui::TableNextColumn(); tab_button(ICON_FA_MICROCHIP, 0);
			ImGui::TableNextColumn(); tab_button(ICON_FA_CUBE, 1);
			ImGui::TableNextColumn(); tab_button(ICON_FA_DRAW_POLYGON, 2);
			ImGui::TableNextColumn(); tab_button(ICON_FA_GEM, 3);
			ImGui::TableNextColumn(); tab_button(ICON_FA_LIGHTBULB, 4);
			ImGui::TableNextColumn(); tab_button(ICON_FA_CAMERA, 5);
		
			ImGui::EndTable();
		}

		ImGui::Separator();

		if (ImGui::BeginChild("tab space"))
		{
			switch (tab_select)
			{
				case 0:
					ImGui::Text("Renderer settings....");
					break;

				case 1:
					bu::ui::node_properties(*ed.scene, ed.selected_nodes);
					break;

				case 2:
					bu::ui::mesh_info(*ed.scene, ed.selected_nodes);
					break;

				case 3:
					// bu::ui::material_editor()
					break;
			}

			ImGui::EndChild();
		}

		// if (ed.scene)
		// {
		// 	bu::ui::scene_graph(*ed.scene, ed.selected_nodes);
		// 	bu::ui::node_controls(*ed.scene, ed.selected_nodes);		

		// 	if (ImGui::CollapsingHeader("Node properties"))
		// 		bu::ui::node_properties(*ed.scene, ed.selected_nodes);

		// 	if (ImGui::CollapsingHeader("Meshes info"))
		// 		bu::ui::mesh_info(*ed.scene, ed.selected_nodes);
		// }
	}

	ImGui::End();
}

static void editor_ui(bunsen_editor &ed, bool debug = false)
{
	static bool debug_window_open = true;

	// Main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("Load scene", "l", nullptr);
			ImGui::MenuItem("Save scene", "s", nullptr);
			if (ImGui::MenuItem("Import model", "i", nullptr))
				dialog_import_model(ed, true);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (debug)
				ImGui::MenuItem("Evil debug cheats", nullptr, &debug_window_open);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Dialogs and windows
	dialog_import_model(ed);
	window_editor(ed);
	if (debug_window_open && debug) window_debug_cheats(ed);
}

void bunsen_editor::draw(const bu::bunsen_state &main_state)
{
	// Get current window size
	glm::ivec2 window_size;
	glfwGetWindowSize(main_state.window, &window_size.x, &window_size.y);

	// Update camera
	this->viewport_camera.aspect = float(window_size.x) / window_size.y;
	if (!layout_ed.is_transform_pending())
		bu::update_camera_orbiter_from_mouse(this->orbit_manipulator, main_state.user_input);
	this->orbit_manipulator.update_camera(this->viewport_camera);

	// Update layout editor
	layout_ed.update(main_state.user_input, selected_nodes, viewport_camera, glm::vec2{window_size}, overlay);

	// Draw scene in preview mode
	if (this->scene)
	{
		if (this->preview)
			preview->draw(*this->scene, this->viewport_camera, {selected_nodes.begin(), selected_nodes.end()});
	}

	// The UI
	if (!layout_ed.is_transform_pending())
		editor_ui(*this, main_state.debug || main_state.gl_debug);

	// The overlay
	ImGui::Begin("overlay", nullptr,
		ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(window_size.x, window_size.y));
	overlay.draw();
	ImGui::End();
}