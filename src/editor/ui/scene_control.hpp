#pragma once
#include <set>
#include <memory>
#include "../../scene.hpp"
#include "../scene_selection.hpp"

namespace bu::ui {

void scene_graph(const bu::scene &scene, bu::scene_selection &selection);
void node_controls(const bu::scene &scene, bu::scene_selection &selection);
void node_properties(const std::shared_ptr<bu::scene_node> &node);
void node_menu(const bu::scene &scene, bu::scene_selection &selection);
}
