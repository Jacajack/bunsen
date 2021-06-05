#include "widget.hpp"
#include <imgui.h>
using bu::ui::widget;

widget::widget(window &win) :
	m_window(&win)
{
}

// void widget::draw() 
// {
// 	ImGui::TextWrapped("This is widget::draw()");
// }

