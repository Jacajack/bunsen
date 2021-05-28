#include "bvh.hpp"
#include "bvh_builder.hpp"
#include <stack>

using bu::rt::bvh_tree;
using bu::rt::bvh_draft;
using bu::rt::bvh_draft_node;

void bvh_tree::populate(const bvh_draft &draft)
{
	unsigned int t_count = 0;

	std::stack<std::pair<unsigned int, const bvh_draft_node*>> st;
	st.push({1, &draft.get_root_node()});

	while (!st.empty())
	{
		auto [node_id, node_ptr] = st.top();
		st.pop();

		nodes[node_id].aabb = node_ptr->aabb;
		if (node_ptr->triangles.empty())
		{
			if (!node_ptr->left || !node_ptr->right)
				throw std::runtime_error{"BVH draft has empty node with no children - cannot populate final BVH"};

			// This is not a leaf node
			nodes[node_id].count = 0;

			// Check if children overlap
			nodes[node_id].index = node_ptr->left->aabb.check_overlap(node_ptr->right->aabb);

			// Process children
			st.push({node_id * 2, node_ptr->left.get()});
			st.push({node_id * 2 + 1, node_ptr->right.get()});
		}
		else // Leaf node
		{
			if (node_ptr->left || node_ptr->right)
				LOG_ERROR << "BVH draft node has both triangles and children! (bvh_tree::populate())";

			// Write triangle indices
			nodes[node_id].index = t_count;
			nodes[node_id].count = node_ptr->triangles.size();

			// Copy triangles
			for (const auto &t : node_ptr->triangles)
				triangles[t_count++] = t;
		}	
	}
}