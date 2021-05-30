#pragma once
#include <glm/glm.hpp>

namespace bu::rt {

struct ray
{
	glm::vec3 origin;
	glm::vec3 direction;
};

struct triangle
{
	glm::vec3 vertices[3];
	glm::vec3 normals[3];
	glm::vec2 uvs[3];

	// rt::material *material;
};

struct triangle_hit
{
	float t, u, v;
};

struct ray_hit
{
	float t, u, v;
	const rt::triangle *triangle;
};

inline glm::vec3 ray_hit_pos(const ray &r, const ray_hit &h)
{
	return r.origin + h.t * r.direction;
}

inline glm::vec3 ray_hit_normal(const ray &r, const ray_hit &h)
{
	return glm::normalize(
		(1.f - h.u - h.v) * h.triangle->normals[0]
		+ h.u * h.triangle->normals[1]
		+ h.v * h.triangle->normals[2]
		);
}

/**
	\note Based on: https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
*/
inline bool ray_intersect_triangle(const ray &r, const triangle &tri, float &t, float &u, float &v)
{
	glm::vec3 E1 = tri.vertices[1] - tri.vertices[0];
	glm::vec3 E2 = tri.vertices[2] - tri.vertices[0];
	glm::vec3 P = glm::cross(r.direction, E2);
	float det = glm::dot(P, E1);
	if (det == 0.0)
		return false;

	//! \todo Backface culling

	glm::vec3 T = r.origin - tri.vertices[0];
	glm::vec3 Q = glm::cross(T, E1);
	glm::vec3 tuv = glm::vec3{glm::dot(Q, E2), glm::dot(P, T), glm::dot(Q, r.direction)} * (1.f / det);
	t = tuv.x;
	u = tuv.y;
	v = tuv.z;

	// Check u bounds
	if (u < 0.f || u > 1.f)
		return false;

	// Check v bounds
	if (v < 0.f || u + v > 1.f)	
		return false;

	// Reject negative distances
	if (t < 0.f)
		return false;

	return true;
}

}