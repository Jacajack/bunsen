#pragma once
#include <glm/glm.hpp>
#include "core.hpp"
#include "aabb.hpp"

namespace bu::rt {

/**
	\note If count is negative, this is not a child node. In such case, if index
	is non-zero, the chlidren nodes do overlap.
*/
struct bvh_node
{
	rt::aabb aabb;
	unsigned int index;
	int count;
};

struct bvh_tree
{
	bvh_tree(unsigned int nodes, unsigned int tris);
	~bvh_tree();

	triangle *triangles;
	bvh_node *nodes;
	unsigned int node_count;
	unsigned int triangle_count;

	bool test_ray(const rt::ray &r, rt::ray_hit &hit) const;
};


}