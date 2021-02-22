#pragma once
#include <set>
#include <memory>
#include "../../mesh.hpp"
#include "../../scene_selection.hpp"

namespace bu::ui {

void mesh_info(const std::shared_ptr<bu::mesh> &mesh_data);
void model_menu(bu::scene_selection &selection);

}