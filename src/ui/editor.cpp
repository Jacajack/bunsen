#include "editor.hpp"
#include <stack>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
#include <imgui_icon_font_headers/IconsFontAwesome5.h>
#include <tracy/Tracy.hpp>

#include "assimp_loader.hpp"
#include "scene_export.hpp"
#include "log.hpp"

#include "ui/ui.hpp"
#include "ui/window.hpp"

#include "ui/windows/debug_window.hpp"
#include "ui/windows/scene_editor_window.hpp"
#include "ui/windows/rendered_view_window.hpp"
#include "ui/windows/imgui_style_editor_window.hpp"

#include "renderers/preview/preview.hpp"
#include "renderers/preview/basic_preview.hpp"
#include "renderers/albedo/albedo.hpp"
#include "renderers/rt/rt.hpp"

using bu::bunsen_editor;

bunsen_editor::bunsen_editor() :
	scene(std::make_shared<bu::scene>()),
	preview_ctx(std::make_shared<bu::preview_context>()),
	albedo_ctx(std::make_shared<bu::albedo_context>()),
	rt_ctx(std::make_shared<bu::rt_context>(*scene->event_bus, preview_ctx))
{
	windows.push_back(std::make_unique<ui::rendered_view_window>(*this, -1, scene->event_bus));
	windows.push_back(std::make_unique<ui::scene_editor_window>(*this, scene->event_bus));
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
			// if (ImGui::MenuItem("Import model", "i", nullptr))
				// dialog_import_model(*this, true);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("3D view"))
			{
				windows.push_back(std::make_unique<ui::rendered_view_window>(*this, -1, scene->event_bus));
			}

			if (ImGui::MenuItem("Scene editor"))
				windows.push_back(std::make_unique<ui::scene_editor_window>(*this, scene->event_bus));

			if (debug)
				if (ImGui::MenuItem("Evil debug cheats"))
					windows.push_back(std::make_unique<ui::debug_window>(*this));

			if (debug)
				if (ImGui::MenuItem("ImGui style editor"))
					windows.push_back(std::make_unique<ui::imgui_style_editor_window>());

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// Draw all open windows
	
	{
	ZoneScopedNC("UI windows", 0xff8888ff)
	windows.erase(std::remove_if(windows.begin(), windows.end(), [](auto &w){return !w->is_open();}), windows.end());
	for (auto &w : windows)
		w->display();
	}


	// TEMP update BVH
	bool tp = scene->layout_ed.is_transform_pending();
	rt_ctx->update_from_scene(*scene, !tp);

	// static auto events = scene->event_bus->make_connection();
	// bu::event ev;
	// while (events->poll(ev))
	// {
	// 	LOG_DEBUG << "GOT EVENT";
	// }

	if (scene->root_node->is_modified())
		LOG_DEBUG << "scene root is modified";
	else if (scene->root_node->is_visibly_modified())
		LOG_DEBUG << "scene root is visibly modified";

	scene->root_node->clear_modified();

	// TEMP unbuffer meshes
	preview_ctx->unbuffer_outdated_meshes();
}