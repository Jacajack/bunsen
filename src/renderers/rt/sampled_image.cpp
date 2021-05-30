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
		const auto &splat = bucket.data[i];

		glm::ivec2 A{glm::floor(splat.pos)};
		glm::ivec2 B{A + 1};
		glm::vec2 t{glm::mod(splat.pos, 1.f)};

		if (A.x >= 0 && A.y >= 0 && A.x < size.x && A.y < size.y)
			at(A) += glm::vec4{splat.color, splat.samples * (1.f - t.x) * (1.f - t.y)};

		if (B.x >= 0 && A.y >= 0 && B.x < size.x && A.y < size.y)
			at(glm::ivec2{B.x, A.y}) += glm::vec4{splat.color, splat.samples * t.x * (1.f - t.y)};

		if (B.x >= 0 && B.y >= 0 && B.x < size.x && B.y < size.y)
			at(B) += glm::vec4{splat.color, splat.samples * t.x * t.y};

		if (A.x >= 0 && B.y >= 0 && A.x < size.x && B.y < size.y)
			at(glm::ivec2{A.x, B.y}) += glm::vec4{splat.color, splat.samples * (1.f - t.x) * t.y};
	}
}

void sampled_image::clear()
{
	for (auto &pixel : data)
		pixel = glm::vec4{0.f};
}