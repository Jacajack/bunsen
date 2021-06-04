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
}