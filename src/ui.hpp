#pragma once

#include "bunsen.hpp"

namespace bu {

struct ui_state
{
	const scene_node *selected_node;
};

void draw_ui(ui_state &state, const bu::bunsen_state &main_state);

}
