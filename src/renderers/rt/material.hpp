#pragma once
#include "ray.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace bu {
struct material_data;
}

namespace bu::rt {

enum class material_type
{
	BASIC_DIFFUSE,
	EMISSIVE
};

struct material
{
	material();
	material(const bu::material_data &mat);
	
	inline ray_bounce sample(const glm::vec3 &V, float ior, float u1, float u2) const;
	inline ray_bounce sample_basic_diffuse(const glm::vec3 &V, float ior, float u1, float u2) const;
	inline ray_bounce sample_emissive() const;

	material_type type;

	struct
	{
		glm::vec3 albedo;
	} basic_diffuse;

	struct
	{
		glm::vec3 emission;
	} emissive;
};

ray_bounce material::sample(const glm::vec3 &V, float ior, float u1, float u2) const
{
	switch (this->type)
	{
		default:
		case material_type::BASIC_DIFFUSE:
			return sample_basic_diffuse(V, ior, u1, u2);
			break;

		case material_type::EMISSIVE:
			return sample_emissive();
			break;
	}
}

/**
	\brief Lambertian material
*/
ray_bounce material::sample_basic_diffuse(const glm::vec3 &V, float ior, float u1, float u2) const
{
	ray_bounce bounce;
	bounce.type = ray_bounce_type::DIFFUSE;
	bounce.new_ior = ior;
	bounce.bsdf = basic_diffuse.albedo;
	
	float phi = u1 * 2.f * glm::pi<float>();
	float cos_theta = std::sqrt(u2);
	float sin_theta = std::sqrt(1.f - u2);
	bounce.new_direction = glm::vec3{
		std::cos(phi) * sin_theta,
		std::sin(phi) * sin_theta,
		cos_theta
		};

	bounce.pdf = 1.f;
	return bounce;
}

ray_bounce material::sample_emissive() const
{
	ray_bounce bounce;
	bounce.type = ray_bounce_type::EMISSION;
	bounce.new_ior = 0.f;
	bounce.bsdf = emissive.emission;
	bounce.pdf = 1.f;
	return bounce;
}

}