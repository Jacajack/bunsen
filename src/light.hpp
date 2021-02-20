#pragma once
#include <string>
#include <glm/glm.hpp>

namespace bu {

/**
	\brief Abstract base for all lights
*/
struct light
{
	std::string m_name;

	virtual glm::vec3 get_intensity(const glm::vec3 &direction) const
	{
		return glm::vec3{0.f};
	}

	virtual ~light() = default;
};

/**
	\brief An isotropic point (spherical) light source
*/
struct point_light : public light
{
	glm::vec3 color = glm::vec3{1};
	float power = 1;
	float radius = 0;
};

}