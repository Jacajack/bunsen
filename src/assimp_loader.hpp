#pragma once

#include "scene.hpp"
#include <memory>

namespace bu {

std::shared_ptr<bu::scene_node> load_mesh_from_file(const std::string &path);

}