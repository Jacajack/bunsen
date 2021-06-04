#pragma once
#include "../material.hpp"
#include <glm/glm.hpp>

namespace bu {

struct glass_material : public surface_material
{
	glm::vec3 color{1.f};
	float ior{1.5f};
};

}