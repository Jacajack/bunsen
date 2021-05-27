#pragma once
#include <glm/glm.hpp>

namespace bu::rt {

struct triangle
{
	glm::vec3 positions[3];
	glm::vec3 normals[3];
	glm::vec2 uvs[3];

	// rt::material *material;
};

struct ray
{
	glm::vec3 origin;
	glm::vec3 direction;
};

struct ray_triangle_intesection
{
	float t;
	float u, v;
};

struct ray_hit
{
	ray r;
	// ray_intesection i;

	glm::vec3 position;
};


}