#pragma once
#include <set>
#include <memory>
#include "../../scene.hpp"

namespace bu::ui {

void mesh_data_info(std::shared_ptr<bu::mesh_data> mesh_data);
void mesh_menu(const bu::scene &scene, std::set<std::shared_ptr<bu::scene_node>> &selection);

}