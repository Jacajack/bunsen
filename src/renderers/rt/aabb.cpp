#include "aabb.hpp"
#include <tracy/Tracy.hpp>

bu::rt::aabb bu::rt::triangle_aabb(const bu::rt::triangle &t)
{
	ZoneScopedN("triangle_aabb");
	
	bu::rt::aabb box;
	box.min = glm::min(glm::min(t.positions[0], t.positions[1]), t.positions[2]);
	box.max = glm::max(glm::max(t.positions[0], t.positions[1]), t.positions[2]);
	return box;
}

bu::rt::aabb bu::rt::triangles_aabb(const bu::rt::triangle *arr, unsigned int size)
{
	ZoneScopedN("triangles_aabb");

	if (!size)
		return {};
	
	bu::rt::aabb box = triangle_aabb(arr[0]);
	for (auto i = 1u; i < size; i++)
		box.add_aabb(triangle_aabb(arr[i]));

	return box;
}