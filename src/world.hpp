#pragma once
#include <glm/glm.hpp>

namespace bu {

/**
	\brief Base class for all world types
*/
struct world 
{
	virtual ~world() = default;
};

struct solid_world : public world
{
	glm::vec3 color = glm::vec3{0.1f};
};

}