#pragma once

namespace br {

struct triangle
{
	glm::vec3 vertices[3];
	glm::vec3 normals[3];
};

struct mesh
{
	std::vector<triangle> triangles;
	std::vector<material> materials;
};

struct scene
{
	std::vector<mesh> meshes;
	std::vector<camera> cameras;
	std::vector<light> lights;
};

}