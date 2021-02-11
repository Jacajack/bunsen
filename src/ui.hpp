#pragma once

#include "borealis.hpp"

namespace br {

struct ui_state
{
	const scene_node *selected_node;
};

void draw_ui(ui_state &state, const br::borealis_state &main_state);

}
