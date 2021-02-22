#pragma once
#include <set>
#include <memory>
#include "../../light.hpp"
#include "../../scene_selection.hpp"

namespace bu::ui {

void light_editor(const std::shared_ptr<bu::light> &light);
void light_menu(bu::scene_selection &selection);

}