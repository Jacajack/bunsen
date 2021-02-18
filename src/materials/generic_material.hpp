#pragma once
#include "../material.hpp"
#include <glm/glm.hpp>

namespace bu {

/**
	\brief The most generic and versatile material for PBR
*/
struct generic_material : public surface_material
{
	glm::vec3 color = glm::vec3{0.8};
	glm::vec3 emission = glm::vec3{0.f};
	float roughness = 0.5f;
	float metallic = 0.0f;
	float transmission = 0.0f;
	float ior = 1.5f;
};

}