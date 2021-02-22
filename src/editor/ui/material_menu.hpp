#pragma once
#include <memory>
#include <vector>
#include "../../material.hpp"
#include "../../scene_selection.hpp"

namespace bu::ui {

void material_editor(std::shared_ptr<bu::material_data> mat);
void material_menu(bu::scene_selection &selection);

}