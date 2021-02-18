#pragma once
#include <string>
#include <glm/glm.hpp>

namespace bu {

/**
	\brief Abstract base for all lights
*/
struct light_data
{
	std::string m_name;

	virtual glm::vec3 get_intensity(const glm::vec3 &direction) const;
};

}