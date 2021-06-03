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
	GLASS,
	EMISSIVE
};

struct material
{
	material();
	material(const bu::material_data &mat);
	
	inline ray_bounce sample(const glm::vec3 &V, float ior, float u1, float u2) const;
	inline ray_bounce sample_basic_diffuse(const glm::vec3 &V, float ior, float u1, float u2) const;
	inline ray_bounce sample_glass(const glm::vec3 &V, float ior, float u1) const;
	inline ray_bounce sample_emissive() const;

	material_type type;

	struct
	{
		glm::vec3 albedo;
	} basic_diffuse;

	struct
	{
		glm::vec3 color;
		float ior;
	} glass;

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

		case material_type::GLASS:
			return sample_glass(V, ior, u1);
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

ray_bounce material::sample_glass(const glm::vec3 &V, float ior, float u1) const
{
	float n1, n2;

	// Determine if the ray is entering or leaving the medium
	if (V.z < 0)
	{
		n1 = ior;
		n2 = glass.ior;
	}
	else
	{
		n1 = glass.ior;
		n2 = ior;
	}

	// Calculate Fresnel factor by Schlick's approximation
	float cos_theta = std::abs(V.z);
	float r = (n1 - n2) / (n1 + n2);
	float R = r * r;
	float F = R + (1.f - R) * std::pow(1.f - cos_theta, 5.f);

	if (u1 < F)
	{
		// Reflection
		ray_bounce bounce;
		bounce.type = ray_bounce_type::SPECULAR;
		bounce.new_ior = n1;
		bounce.bsdf = glm::vec3{1.f};
		bounce.new_direction = V * glm::vec3{1, 1, -1};
		bounce.pdf = 1.f;
		return bounce;
	}
	else
	{
		// Transmission
		glm::vec3 T = glm::refract(V, glm::vec3{0, 0, -glm::sign(V.z)}, n1 / n2);
		if (T != glm::vec3{0.f})
		{
			// Refraction
			ray_bounce bounce;
			bounce.type = ray_bounce_type::TRANSMISSION;
			bounce.new_ior = n2;
			bounce.bsdf = glass.color;
			bounce.new_direction = T;
			bounce.pdf = 1.f;
			return bounce;
		}
		else
		{
			// Total Interal Reflection
			ray_bounce bounce;
			bounce.type = ray_bounce_type::SPECULAR;
			bounce.new_ior = n1;
			bounce.bsdf = glm::vec3{1.f};
			bounce.new_direction = V * glm::vec3{1, 1, -1};
			bounce.pdf = 1.f;
			return bounce;
		}
	}
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