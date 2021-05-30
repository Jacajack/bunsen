#pragma once 
#include <glm/glm.hpp>
#include <random>

namespace bu::rt {
class bvh_tree;
struct ray;

glm::vec3 trace_ray(const bu::rt::bvh_tree &bvh, std::mt19937 &rng, bu::rt::ray r, int max_bounces);

}