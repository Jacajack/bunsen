#pragma once
#include <list>
#include <memory>
#include "../../scene.hpp"

namespace bu::ui {

void scene_graph(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection);
void node_controls(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection);
void node_properties(const bu::scene &scene, std::list<std::weak_ptr<bu::scene_node>> &selection);

}
