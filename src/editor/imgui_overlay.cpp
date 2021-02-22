#include "imgui_overlay.hpp"
#include "../utils.hpp"

using bu::imgui_overlay;

static ImU32 color_to_imgui(const glm::vec4 &col)
{
	ImU32 c = 0;
	c |= int(col.a * 255) << 24;
	c |= int(col.b * 255) << 16;
	c |= int(col.g * 255) << 8;
	c |= int(col.r * 255) << 0;
	return c;
}

void imgui_overlay::add_line(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &color, float width)
{
	m_lines.push_back({a, b, color_to_imgui(color), width});
}

void imgui_overlay::add_3d_line(glm::vec4 a, glm::vec4 b, const glm::vec4 &color, float width)
{
	a /= a.w;
	b /= b.w;
	add_line(glm::vec2{a}, glm::vec2{b}, color, width);
}

void imgui_overlay::draw()
{
	auto wp = ImGui::GetWindowPos();
	auto cr = ImGui::GetWindowContentRegionMin();
	auto ws = ImGui::GetContentRegionAvail();
	auto to_window = [&](const glm::vec2 &p)
	{
		auto v = bu::to_vec2(wp) + bu::to_vec2(cr) + (glm::vec2(p.x, -p.y) * 0.5f + 0.5f) * glm::vec2(ws.x, ws.y);
		return ImVec2(v.x, v.y);
	};

	for (auto &l : m_lines)
		ImGui::GetWindowDrawList()->AddLine(to_window(l.a), to_window(l.b), l.color, l.width);
	m_lines.clear();
}