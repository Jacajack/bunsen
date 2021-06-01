#pragma once
#include "ui/window.hpp"

namespace bu {
struct bunsen_editor;
}

namespace bu::ui {

class debug_window : public bu::ui::window
{
public:
	debug_window(bunsen_editor &editor) :
		window("Evil Debug Cheats"),
		m_editor(editor)
	{}

	void draw() override;

private:
	float m_color[3] = {0};
	bunsen_editor &m_editor;
};

}