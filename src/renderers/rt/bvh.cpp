#include "bvh.hpp"
#include "linear_stack.hpp"

using bu::rt::bvh_tree;

bvh_tree::bvh_tree(unsigned int height, unsigned int tris) :
	node_count(std::exp2(height)),
	triangle_count(tris)
{
	// We're doing this ugly way for now, because cudaMalloc() is no prettier
	triangles = reinterpret_cast<triangle*>(calloc(sizeof(triangle), triangle_count));
	nodes = reinterpret_cast<bvh_node*>(calloc(sizeof(bvh_node), node_count));
}

bvh_tree::~bvh_tree()
{
	free(triangles);
	free(nodes);
}

bool bvh_tree::test_ray(const rt::ray &r, rt::ray_hit &hit) const
{
	bu::rt::linear_stack<unsigned int, 32> st;
	
	// Check intersection with the root node
	{
		float t;
		if (nodes[1].aabb.test_ray(r, t))
			st.push(1);
		else
			return false;
	}

	ray_hit best;
	best.t = HUGE_VALF;
	best.triangle = nullptr;

	while (!st.empty())
	{
		// Get node
		auto node_id = st.top();
		auto &node = nodes[node_id];
		st.pop();

		// Positive triangle count indicates a leaf node
		if (node.count <= 0)
		{
			// If index is non-zero, children nodes overlap
			bool children_overlap = node.index;

			auto id_l = 2 * node_id;
			auto id_r = 2 * node_id + 1;
			auto &nl = nodes[id_l];
			auto &nr = nodes[id_r];

			float tl, tr;
			bool hit_l = nl.aabb.test_ray(r, tl);
			bool hit_r = nr.aabb.test_ray(r, tr);

			if (hit_l && hit_r) // Both children were hit
			{
				// If children do not overlap, check only the closer one
				if (!children_overlap)
				{
					if (tl < tr)
						st.push(id_l);
					else
						st.push(id_r);
				}
				else
				{
					// If children overlap check the closer one first
					if (tl < tr)
					{
						st.push(id_r);
						st.push(id_l);
					}
					else
					{
						st.push(id_l);
						st.push(id_r);
					}
				}
			}
			else if (hit_l)
				st.push(id_l);
			else if (hit_r)
				st.push(id_r);
		}
		else // Leaf node
		{
			for (unsigned int i = node.index; i < node.index + node.count; i++)
			{
				float t, u, v;
				if (ray_intersect_triangle(r, triangles[i], t, u, v) && t < best.t)
				{
					best.t = t;
					best.u = u;
					best.v = v;
					best.triangle = &triangles[i];
				}
			}
		}
	}

	if (best.t != HUGE_VALF)
	{
		hit = best;
		return true;
	}
	else
		return false;
}