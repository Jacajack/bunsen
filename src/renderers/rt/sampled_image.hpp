#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace bu::rt {

/**
	\brief Result of sampling a ray
*/
struct pixel_splat
{
	glm::vec3 color;
	glm::vec2 pos;
	float samples;
};

/**
	\note This structure has to be able to handle memory allocated for CUDA device
*/
struct splat_bucket
{
	splat_bucket(size_t s);
	~splat_bucket();

	pixel_splat *data;
	size_t size;
};


/**

*/
struct sampled_image
{
	glm::ivec2 size;
	std::vector<glm::vec4> data;

	sampled_image(glm::ivec2 size);

	glm::vec4 &at(glm::ivec2 pos)
	{
		return data[pos.x + pos.y * size.x];
	}

	void splat(splat_bucket &bucket);
	void clear();
};

}