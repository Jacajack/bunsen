#include "aabb.hpp"
#include <tracy/Tracy.hpp>

bu::rt::aabb bu::rt::triangle_aabb(const bu::rt::triangle &t)
{
	ZoneScopedN("triangle_aabb");
	
	bu::rt::aabb box;
	box.max = glm::max(glm::max(t.vertices[0], t.vertices[1]), t.vertices[2]);
	box.min = glm::min(glm::min(t.vertices[0], t.vertices[1]), t.vertices[2]);
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