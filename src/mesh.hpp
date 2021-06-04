#pragma once
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "uid_provider.hpp"

namespace bu {

/**
	\brief Mesh data contains and owns purely geometrical mesh data
*/
struct mesh : public uid_provider<mesh>
{
	std::string name;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> uvs;
	std::vector<unsigned int> indices;
};

}