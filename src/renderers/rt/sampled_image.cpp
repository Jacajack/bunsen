#include "sampled_image.hpp"
#include <tracy/Tracy.hpp>

using bu::rt::pixel_splat;
using bu::rt::splat_bucket;
using bu::rt::sampled_image;

splat_bucket::splat_bucket(size_t s) :
	size(s)
{
	data = new pixel_splat[size];
}

splat_bucket::~splat_bucket()
{
	delete[] data;
}

sampled_image::sampled_image(glm::ivec2 s) :
	size(s),
	data(s.x * s.y)
{
}

void sampled_image::splat(splat_bucket &bucket)
{
	ZoneScoped;
	
	for (auto i = 0u; i < bucket.size; i++)
	{
		auto &splat = bucket.data[i];

		glm::ivec2 pos{splat.pos};
		if (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) continue;
		at(pos) += glm::vec4{splat.color, splat.samples};
	}
}