#include "debug_window.hpp"
#include <imgui.h>
#include "ui/ui.hpp"
#include "ui/editor.hpp"
#include "renderers/albedo/albedo.hpp"
#include "renderers/preview/preview.hpp"
#include "renderers/rt/rt.hpp"
using bu::ui::debug_window;

void debug_window::draw()
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
		*m_editor.rt_ctx = bu::rt_context(*m_editor.scene->event_bus, m_editor.preview_ctx);

	// static char buf[128];
	// static bool focus = false;
	// static bool want = false;
	// static bool ed = false;
	// bool accept = false;

	// if (ImGui::InputText("test", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue))
	// 	accept = true;

	// accept |= ImGui::IsItemDeactivatedAfterEdit();

	// bool newed = ImGui::IsItemEdited();

	// bool newfocus = ImGui::IsItemFocused();
	// bool newwant = ImGui::GetIO().WantCaptureKeyboard;
	// if (!newfocus && focus)
	// 	accept = true;
	// if (!newwant && want)
	// 	accept = true;
	// want = newwant;
	// focus = newfocus;
		
	// if (accept)
	// 	LOG_WARNING << buf;



	static char buf[128];
	ImGui::InputText("test", buf, 128);
	if (ImGui::Button("add"))
	{
		m_mgr.add(buf, std::make_unique<test_resource>(std::string{"Asd"}));
		m_mgr.emplace(buf, std::string{"Asd"});
	}

	if (ImGui::BeginTable("Mesh info", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
	{
		auto ids = m_mgr.get_ids();
		for (auto id : ids)
		{
			auto handle = m_mgr.get(id);

			ImGui::TableNextColumn();
			ImGui::Text("%ld", handle->resource_id());
			ImGui::TableNextColumn();
			// ImGui::Text("%s", handle->get_name().c_str());
			ImGui::Text("%s", handle->r()->data.c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%d", handle->handle_count());
		}

		ImGui::EndTable();
	}
}