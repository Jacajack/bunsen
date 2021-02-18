#pragma once
#include "../material.hpp"
#include <glm/glm.hpp>

namespace bu {

struct diffuse_material : public surface_material
{
	glm::vec3 color;
};


}