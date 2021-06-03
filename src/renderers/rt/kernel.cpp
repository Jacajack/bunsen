#include "kernel.hpp"
#include "ray.hpp"
#include "bvh.hpp"
#include "material.hpp"

#include <glm/gtx/component_wise.hpp>

glm::vec3 bu::rt::trace_ray(
	const bu::rt::bvh_tree &bvh,
	const std::vector<bu::rt::material> &materials,
	std::mt19937 &rng,
	bu::rt::ray r,
	int max_bounces)
{
	std::uniform_real_distribution<float> dist(0, 1);
	glm::vec3 L{0.0};
	glm::vec3 thrput{1.0};
	float ray_ior = 1.f;

	for (int bounces = 0; bounces < max_bounces; bounces++)
	{
		ray_hit hit;
		bool did_hit = bvh.test_ray(r, hit);

		// World hit
		if (!did_hit)
		{
			L += thrput * glm::pow(glm::max(0.f, glm::dot(r.direction, glm::normalize(glm::vec3{1, 1, 1}))), 12.f) * 5.f;
			break;
		}

		// Compute position, normal and find a tangent and bitangent
		glm::vec3 P = bu::rt::ray_hit_pos(r, hit);
		glm::vec3 N = bu::rt::ray_hit_normal(r, hit);
		glm::vec3 T = glm::normalize(glm::cross(r.direction, N));
		glm::vec3 B = glm::normalize(glm::cross(T, N));
		glm::mat3 TBN{T, B, N};
		glm::mat3 inv_TBN{glm::transpose(TBN)};

		// Transform ray direction to tangent space
		glm::vec3 tbn_dir = inv_TBN * r.direction;

		// Sample the BSDF
		const auto &material = materials[hit.triangle->material_id];
		auto bounce = material.sample(tbn_dir, ray_ior, dist(rng), dist(rng));

		// Stop if we hit a light
		if (bounce.type == ray_bounce_type::EMISSION)
		{
			L += thrput * bounce.bsdf;
			break;
		}

		// The new ray
		const float ray_surface_offset = 1e-3;
		r.direction = glm::normalize(TBN * bounce.new_direction);
		r.origin = P + glm::sign(bounce.new_direction.z) * ray_surface_offset * N;
		ray_ior = bounce.new_ior;

		// Accumulate light
		thrput *= bounce.bsdf / bounce.pdf;

		// Russian roulette
		float p_survive = glm::compMax(thrput);
		if (dist(rng) > p_survive)
			break;
		thrput *= 1 / p_survive;
	}

	return L;
}