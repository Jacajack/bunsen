#include "editor.hpp"
#include <stack>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
#include <imgui_icon_font_headers/IconsFontAwesome5.h>

#include "../assimp_loader.hpp"
#include "../scene_export.hpp"
#include "../log.hpp"
#include "ui/ui.hpp"
#include "ui/scene_control.hpp"
#include "ui/light_menu.hpp"
#include "ui/material_menu.hpp"
#include "ui/model_menu.hpp"
#include "ui/world_menu.hpp"

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
			"GLTF (*.gltf *.glb){.gltf,.glb}"
			"OBJ files (*.obj){.obj},"
			"Collada files (*.dae){.dae},",
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
	if (!ed.scene)
	{
		LOG_ERROR << "The editor won't be displayed without a loaded scene!";
		return;
	}

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
	
		bu::ui::scene_graph(*ed.scene, ed.selection);
		ImGui::Separator();
		bu::ui::node_controls(*ed.scene, ed.selection);		

		ImGui::Separator();

		// Check what type of node is selected so proper tabs can be dimmed out
		auto primary = ed.selection.get_primary();
		bool primary_valid = bool(primary);
		bool is_model_node = dynamic_cast<bu::model_node*>(primary.get());
		bool is_light_node = dynamic_cast<bu::light_node*>(primary.get());
		bool is_camera_node = dynamic_cast<bu::camera_node*>(primary.get());

		// Tabs
		if (ImGui::BeginTable("tab buttons", 7))
		{
			auto tab_button = [](const char *text, int id, bool enable)
			{
				auto tab_color = ImGui::GetStyleColorVec4(ImGuiCol_Tab);
				auto active_color = ImGui::GetStyleColorVec4(ImGuiCol_TabActive);
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

			ImGui::TableNextColumn(); tab_button(ICON_FA_MICROCHIP, 0, true);
			ImGui::TableNextColumn(); tab_button(ICON_FA_GLOBE_EUROPE, 6, true);
			ImGui::TableNextColumn(); tab_button(ICON_FA_CUBE, 1, primary_valid);
			ImGui::TableNextColumn(); tab_button(ICON_FA_DRAW_POLYGON, 2, is_model_node);
			ImGui::TableNextColumn(); tab_button(ICON_FA_GEM, 3, is_model_node);
			ImGui::TableNextColumn(); tab_button(ICON_FA_LIGHTBULB, 4, is_light_node);
			ImGui::TableNextColumn(); tab_button(ICON_FA_VIDEO, 5, is_camera_node);
		
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
					bu::ui::node_menu(*ed.scene, ed.selection);
					break;

				case 2:
					bu::ui::model_menu(ed.selection);
					break;

				case 3:
					bu::ui::material_menu(ed.selection);
					break;

				case 4:
					bu::ui::light_menu(ed.selection);
					break;

				case 5:
					ImGui::Text("Camera properties...");
					break;

				case 6:
					if (ed.scene->world)
						bu::ui::world_menu(*ed.scene->world);
					break;
			}

			
		}

		ImGui::EndChild();
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
			if (ImGui::MenuItem("Save scene", "s", nullptr))
				LOG_INFO << "Scene:\n" << bu::export_scene(*ed.scene).dump();
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
	layout_ed.update(main_state.user_input, selection, viewport_camera, glm::vec2{window_size}, overlay);

	// Draw scene in preview mode
	if (this->scene)
	{
		if (this->preview)
			preview->draw(*this->scene, this->viewport_camera, selection.get_nodes());
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