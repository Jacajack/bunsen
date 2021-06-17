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
	float nI, nT;

	// Determine if the ray is entering or leaving the medium
	if (V.z < 0)
	{
		// Entering
		nI = ior;
		nT = glass.ior;
	}
	else
	{
		// Leaving
		nI = glass.ior;
		nT = 1.f; // FIXME - we need to handle nested media
	}

	// Calculate transmitted ray
	glm::vec3 T = glm::refract(V, glm::vec3{0, 0, -glm::sign(V.z)}, nI / nT);

	// Total internal reflection
	if (T == glm::vec3{0.f})
	{
		ray_bounce bounce;
		bounce.type = ray_bounce_type::SPECULAR;
		bounce.new_ior = nI;
		bounce.bsdf = glm::vec3{1.f};
		bounce.new_direction = V * glm::vec3{1, 1, -1};
		bounce.pdf = 1.f;
		return bounce;
	}

	// Fresnel factor
	float cosI = std::abs(V.z);
	float cosT = std::abs(T.z); 
	float rparl = (nT * cosI - nI * cosT) / (nT * cosI + nI * cosT);
	float rperp = (nI * cosI - nT * cosT) / (nI * cosI + nT * cosT);
	float F = (rparl * rparl + rperp * rperp) / 2.f;

	if (u1 < F)
	{
		// Reflection
		ray_bounce bounce;
		bounce.type = ray_bounce_type::SPECULAR;
		bounce.new_ior = nI;
		bounce.bsdf = glm::vec3{1.f};
		bounce.new_direction = V * glm::vec3{1, 1, -1};
		bounce.pdf = 1.f;
		return bounce;
	}
	else
	{
		// Refraction
		ray_bounce bounce;
		bounce.type = ray_bounce_type::TRANSMISSION;
		bounce.new_ior = nT;
		bounce.bsdf = glass.color;
		bounce.new_direction = T;
		bounce.pdf = 1.f;
		return bounce;
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