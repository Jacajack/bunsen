#pragma once
#include "../material.hpp"
#include <glm/glm.hpp>

namespace bu {

struct emmisive_material : public surface_material
{
	glm::vec3 color;
};

}