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
#include "ui/rendered_view.hpp"

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

class scene_editor_window : public bu::ui::window
{
public:
	scene_editor_window(bunsen_editor &editor) :
		window("Editor", ImGuiWindowFlags_MenuBar),
		m_editor(editor)
	{}

	void draw() override
	{
		auto &ed = m_editor;
		dialog_import_model(m_editor);

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

		bu::ui::scene_graph(*ed.scene, ed.scene->selection);
		ImGui::Separator();
		bu::ui::node_controls(*ed.scene, ed.scene->selection);		

		ImGui::Separator();

		// Check what type of node is selected so proper tabs can be dimmed out
		auto primary = ed.scene->selection.get_primary();
		bool primary_valid = bool(primary);
		bool is_model_node = dynamic_cast<bu::model_node*>(primary.get());
		bool is_light_node = dynamic_cast<bu::light_node*>(primary.get());
		bool is_camera_node = dynamic_cast<bu::camera_node*>(primary.get());

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
					bu::ui::node_menu(*ed.scene, ed.scene->selection);
					break;

				case 2:
					bu::ui::model_menu(ed.scene->selection);
					break;

				case 3:
					bu::ui::material_menu(ed.scene->selection);
					break;

				case 4:
					bu::ui::light_menu(ed.scene->selection);
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

private:
	bunsen_editor &m_editor;
};

class debug_window : public bu::ui::window
{
public:
	debug_window(bunsen_editor &editor) :
		window("Evil Debug Cheats"),
		m_editor(editor)
	{}

	void draw() override
	{
		if (ImGui::ColorEdit3("Color theme base", &m_color[0]))
		{
			bu::ui::load_theme(m_color[0], m_color[1], m_color[2]);
		}

		if (ImGui::Button("Reload preview renderer"))
			*m_editor.preview_ctx = bu::preview_context();

		if (ImGui::Button("Reload Albedo"))
			*m_editor.albedo_ctx = bu::albedo_context();

		if (ImGui::Button("Reload RT"))
			*m_editor.rt_ctx = bu::rt_context();
	}

private:
	float m_color[3] = {0};
	bunsen_editor &m_editor;
};

class imgui_style_editor_window : public bu::ui::window
{
public:
	imgui_style_editor_window() :
		window("ImGui Style Editor")
	{}

	void draw() override
	{
		ImGui::ShowStyleEditor(&ImGui::GetStyle());
	}
};

bunsen_editor::bunsen_editor() :
	scene(std::make_shared<bu::scene>()),
	preview_ctx(std::make_shared<bu::preview_context>()),
	basic_preview_ctx(std::make_shared<bu::basic_preview_context>()),
	albedo_ctx(std::make_shared<bu::albedo_context>()),
	rt_ctx(std::make_shared<bu::rt_context>())
{
	windows.push_back(std::make_unique<ui::rendered_view_window>(*this));
	windows.push_back(std::make_unique<scene_editor_window>(*this));
}

void bunsen_editor::draw(const bu::bunsen &main_state)
{
	bool debug = main_state.debug || main_state.gl_debug;

	// Main dockspace
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

	// Main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("Load scene", "l", nullptr);
			if (ImGui::MenuItem("Save scene", "s", nullptr))
				LOG_INFO << "Scene:\n" << bu::export_scene(*scene).dump();
			if (ImGui::MenuItem("Import model", "i", nullptr))
				dialog_import_model(*this, true);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("3D view"))
			{
				windows.push_back(std::make_unique<ui::rendered_view_window>(*this));
			}

			if (ImGui::MenuItem("Scene editor"))
				windows.push_back(std::make_unique<scene_editor_window>(*this));

			if (debug)
				if (ImGui::MenuItem("Evil debug cheats"))
					windows.push_back(std::make_unique<debug_window>(*this));

			if (debug)
				if (ImGui::MenuItem("ImGui style editor"))
					windows.push_back(std::make_unique<imgui_style_editor_window>());

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Draw all open windows
	windows.erase(std::remove_if(windows.begin(), windows.end(), [](auto &w){return !w->is_open();}), windows.end());
	for (auto &w : windows)
		w->display();
}