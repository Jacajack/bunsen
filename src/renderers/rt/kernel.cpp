#include "kernel.hpp"
#include "ray.hpp"
#include "bvh.hpp"

#include <glm/gtx/component_wise.hpp>

glm::vec3 bu::rt::trace_ray(
	const bu::rt::bvh_tree &bvh,
	std::mt19937 &rng,
	bu::rt::ray r,
	int max_bounces)
{
	std::uniform_real_distribution<float> dist(0, 1);
	glm::vec3 L{0.0};
	glm::vec3 thrput{1.0};


	for (int bounces = 0; bounces < max_bounces; bounces++)
	{
		ray_hit hit;
		bool did_hit = bvh.test_ray(r, hit);

		// World hit
		if (!did_hit)
		{
			L += thrput * glm::vec3{0.8};
			break;
		}

		// Evaluate BSDF
		glm::vec3 N = bu::rt::ray_hit_normal(r, hit);
		L += thrput * glm::vec3{0.8, 0, 0} * glm::dot(-r.direction, N);
		thrput = glm::vec3{0};

		// Russian roulette
		float p_survive = glm::compMax(thrput);
		if (dist(rng) > p_survive)
			break;
		thrput *= 1 / p_survive;
	}

	return L;
}