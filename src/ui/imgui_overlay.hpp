#pragma once
#include <vector>
#include <imgui.h>
#include <glm/glm.hpp>

namespace bu {

class imgui_overlay
{
public:
	void add_line(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &color = glm::vec4{1.f}, float width = 1.f);
	void add_3d_line(glm::vec4 a, glm::vec4 b, const glm::vec4 &color = glm::vec4{1.f}, float width = 1.f);
	void draw();

private:
	struct line
	{
		glm::vec2 a, b;
		ImU32 color;
		float width;
	};

	std::vector<line> m_lines;
};

}