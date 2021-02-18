#pragma once
#include <list>
#include <memory>
#include "../../scene.hpp"

namespace bu::ui {

void mesh_info(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection);

}