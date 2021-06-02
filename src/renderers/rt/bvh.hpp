#pragma once
#include "aabb.hpp"

namespace bu::rt {

class bvh_draft;

/**
	\note If count is positive, this node contains triangles. If count is zero, this
	node has children. If count is negative, node doesn't have niether children nor triangles.
*/
struct bvh_node
{
	rt::aabb aabb;
	unsigned int index;
	int count;
};

struct bvh_tree
{
	bvh_tree(unsigned int height, unsigned int tris);
	bvh_tree(const bvh_tree &) = delete;
	bvh_tree &operator=(const bvh_tree &) = delete;
	~bvh_tree();

	triangle *triangles = nullptr;
	bvh_node *nodes = nullptr;
	unsigned int node_count = 0;
	unsigned int triangle_count = 0;

	void populate(const bvh_draft &draft);
	bool test_ray(const rt::ray &r, rt::ray_hit &hit) const;
};

}