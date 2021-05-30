#pragma once 
#include <glm/glm.hpp>
#include <random>

namespace bu::rt {
class bvh_tree;
struct ray;
struct material;

glm::vec3 trace_ray(
	const bu::rt::bvh_tree &bvh,
	const std::vector<bu::rt::material> &materials,
	std::mt19937 &rng,
	bu::rt::ray r,
	int max_bounces);

}