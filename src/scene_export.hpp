#pragma once
#include <nlohmann/json.hpp>

namespace bu {

struct scene;

nlohmann::json export_scene(const bu::scene &scene);
bu::scene import_scene(const nlohmann::json &json);

}