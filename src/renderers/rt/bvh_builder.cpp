#include "bvh_builder.hpp"
#include <algorithm>
#include <future>
#include <tracy/Tracy.hpp>
#include "aabb.hpp"
#include "bvh.hpp"
#include "material.hpp"
#include "scene_cache.hpp"
#include "../../log.hpp"
#include "../../async_task.hpp"

using bu::rt::bvh_draft_node;
using bu::rt::bvh_draft;

/**
	\brief A 'box' used in the partitioning process
*/
struct bvh_box
{
	glm::vec3 pos;
	bu::rt::aabb box;
	unsigned int id;
};

static bool partition_boxes_sah(
	const bu::async_stop_flag &stop_flag,
	std::vector<bvh_box> &input, 
	std::vector<unsigned int> &left, 
	std::vector<unsigned int> &right,
	float cost_intersect = 1,
	float cost_traversal = 6)
{
	if (input.size() < 2)
		throw std::runtime_error{"partition_boxes_sah() called on less than 2 objects"};

	ZoneScopedN("Partition BVH");

	auto sort_in_axis = [&](std::vector<bvh_box> &input, float glm::vec3::* axis)
	{
		// LOG_DEBUG << "sorting...";
		ZoneScopedN("Sorting AABB");
		std::sort(input.begin(), input.end(), [axis](auto &a, auto &b)
		{
			return a.pos.*axis < b.pos.*axis;
		});
	};

	auto find_best_split = [&](const std::vector<bvh_box> &input, float &cost, int &index)
	{
		// LOG_DEBUG << "finding best split...";
		ZoneScopedN("Find split");

		bu::rt::aabb_collection l, r;
		index = -1;

		if (input.empty()) return;

		for (auto i = 0u; i < input.size(); i++)
			r.add(input[i].box);

		// "Leave as is cost" - number of primitives times intersection cost
		float sp = r.get_aabb().get_area();
		cost = cost_intersect * r.size();

		for (auto i = 0u; i < input.size() - 1 && !stop_flag.should_stop(); i++)
		{
			l.add(input[i].box);
			r.remove(input[i].box);

			float sl = l.get_aabb().get_area();
			float sr = r.get_aabb().get_area();
			float c = cost_traversal + cost_intersect / sp * (sl * l.size() + sr * r.size());

			// LOG_DEBUG << "split " << i << " sl=" << sl << " sr=" << sr << " c=" << c << " best=" << cost;

			if (c < cost)
			{
				cost = c;
				index = i;
			}
		}

	};

	auto buf_x = input;
	auto buf_y = input;
	auto buf_z = input;

	float cx, cy, cz;
	int ix, iy, iz;
	int best_index{};
	std::vector<bvh_box> *best_buf{};

	sort_in_axis(buf_x, &glm::vec3::x);
	if (stop_flag.should_stop()) return false;
	find_best_split(buf_x, cx, ix);
	if (stop_flag.should_stop()) return false;
	sort_in_axis(buf_y, &glm::vec3::y);
	if (stop_flag.should_stop()) return false;
	find_best_split(buf_y, cy, iy);
	if (stop_flag.should_stop()) return false;
	sort_in_axis(buf_z, &glm::vec3::z);
	if (stop_flag.should_stop()) return false;
	find_best_split(buf_z, cz, iz);
	if (stop_flag.should_stop()) return false;

	if (cx < cy && cx < cz)
	{
		best_index = ix;
		best_buf = &buf_x;
	}
	else if (cy < cx && cy < cz)
	{
		best_index = iy;
		best_buf = &buf_y;
	}
	else
	{
		best_index = iz;
		best_buf = &buf_z;
	}

	// LOG_DEBUG << "best index is " << best_index;
	if (best_index < 0) return false;

	for (int i = 0; i < static_cast<int>(best_buf->size()) && !stop_flag.should_stop(); i++)
	{
		if (i <= best_index)
			left.push_back((*best_buf)[i].id);
		else
			right.push_back((*best_buf)[i].id);
	}

	return !stop_flag.should_stop();
}

/**
	Recursively processes BVH nodes.

	Nodes containing multiple meshes are partitioned and can be split up. If splitting
	is infeasible the, the meshes are dissolved into triangles.

	Nodes containing triangles are recursively partitioned too.

	\todo Split this into more functions
*/
bool process_bvh_node(const bu::async_stop_flag *stop_flag, bu::rt::bvh_draft_node *node, int depth = 0)
{
	if (!node) return false;
	// LOG_DEBUG << "Processing BVH node...";

	/*
		Dissolve nodes where AABBs of all meshes have surface area greater than
		the entire model's AABB. Dissolve nodes containing single meshes too.
	*/
	if (!node->meshes.empty())
	{
		bu::rt::aabb_collection col;
		float total_surf = 0;

		for (auto &mesh : node->meshes)
		{
			col.add(mesh->aabb);
			total_surf = mesh->aabb.get_area();
		}
		
		if (col.get_aabb().get_area() <= total_surf || node->meshes.size() == 1) 
			node->dissolve_meshes();
	}

	if (!node->triangles.empty() && !node->meshes.empty()) 
	{
		LOG_ERROR << "BVH node with both meshes and triangles!";
		throw std::runtime_error("BVH node with both meshes and triangles encountered!");
	}
	else if (!node->triangles.empty()) // Has triangles - partition triangles
	{
		std::vector<bvh_box> contents(node->triangles.size());
		for (auto i = 0u; i < node->triangles.size(); i++)
		{
			auto aabb = bu::rt::triangle_aabb(node->triangles[i]);
			contents[i] = bvh_box{aabb.get_center(), aabb, i};
			
			if (i == 0)
				node->aabb = aabb;
			else
				node->aabb.add_aabb(aabb);
		}

		std::vector<unsigned int> l, r;
		bool should_split = node->triangles.size() > 1 && partition_boxes_sah(*stop_flag, contents, l, r);
		bool split_valid = !l.empty() && !r.empty();

		if (should_split && split_valid)
		{
			// LOG_DEBUG << "splitting into " << l.size() + 1 << " and " << r.size() << " triangles";

			node->left = std::make_unique<bu::rt::bvh_draft_node>();
			node->right = std::make_unique<bu::rt::bvh_draft_node>();

			for (auto id : l)
				node->left->triangles.emplace_back(std::move(node->triangles[id]));
			
			for (auto id : r)
				node->right->triangles.emplace_back(std::move(node->triangles[id]));

			node->triangles.clear();
		}
	}
	else if (!node->meshes.empty()) // Has meshes - try to partition them
	{
		// LOG_DEBUG << "a mesh node...";

		std::vector<bvh_box> contents(node->meshes.size());
		for (auto i = 0u; i < node->meshes.size(); i++)
		{
			auto aabb = node->meshes[i]->aabb;
			contents[i] = bvh_box{aabb.get_center(), aabb, i};
			
			if (i == 0)
				node->aabb = node->meshes[i]->aabb;
			else
				node->aabb.add_aabb(node->meshes[i]->aabb);
		}

		std::vector<unsigned int> l, r;
		bool should_split = node->meshes.size() > 1 && partition_boxes_sah(*stop_flag, contents, l, r);
		bool split_valid = !l.empty() && !r.empty();

		if (should_split && split_valid)
		{
			// LOG_DEBUG << "splitting into " << l.size() + 1 << " and " << r.size() << " meshes";
			node->left = std::make_unique<bu::rt::bvh_draft_node>();
			node->right = std::make_unique<bu::rt::bvh_draft_node>();

			for (auto id : l)
				node->left->meshes.emplace_back(std::move(node->meshes[id]));
			
			for (auto id : r)
				node->right->meshes.emplace_back(std::move(node->meshes[id]));

			node->meshes.clear();
		}
		else // If no split, dissolve meshes and try this node again
		{
			node->dissolve_meshes();
			return !stop_flag->should_stop() && process_bvh_node(stop_flag, node, depth);
		}
	}
	else // Empty node
	{
		return false;
	}

	// Bail out if requested
	if (stop_flag->should_stop())
		return false;

	/*
		Partition recursively and asynchronously on the first 4 levels.
	*/
	if (depth < 4)
	{
		auto async_policy = std::launch::async;

		std::future<bool> lfut, rfut;
		if (node->left) lfut = std::async(async_policy, process_bvh_node, stop_flag, node->left.get(), depth + 1);
		if (node->right) rfut = std::async(async_policy, process_bvh_node, stop_flag, node->right.get(), depth + 1);
		if (node->left) lfut.wait();
		if (node->right) rfut.wait();
	}
	else
	{
		if (node->left) process_bvh_node(stop_flag, node->left.get(), depth + 1);
		if (node->right) process_bvh_node(stop_flag, node->right.get(), depth + 1);
	}

	return true;
}

void bvh_draft_node::dissolve_meshes()
{
	ZoneScopedN("Dissolve meshes in BVH node");

	for (auto &mesh : meshes)
		std::copy(mesh->triangles.begin(), mesh->triangles.end(), std::back_insert_iterator(triangles));
	meshes.clear();
}

std::vector<bu::rt::aabb> bu::rt::bvh_draft::get_tree_aabbs() const
{
	std::vector<aabb> aabbs;

	std::stack<bu::rt::bvh_draft_node*> node_stack;
	node_stack.push(m_root_node.get());
	while (!node_stack.empty())
	{
		auto node = node_stack.top();
		node_stack.pop();

		if (!node) continue;

		if (node->meshes.size() && node->triangles.size())
			LOG_ERROR << "get_tree_aabbs() found node with meshes and triangles";

		aabbs.push_back(node->aabb);
		node_stack.push(node->left.get());
		node_stack.push(node->right.get());
	}

	return aabbs;
}

const bvh_draft_node &bvh_draft::get_root_node() const
{
	return *m_root_node;
}

int bvh_draft_node::height() const
{
	int hl = 0, hr = 0;
	if (left) hl = left->height();
	if (right) hr = right->height();
	return 1 + std::max(hl, hr);
}

int bvh_draft::get_height() const
{
	return get_root_node().height();
}

int bvh_draft::get_triangle_count() const
{
	int n = 0;
	std::stack<bvh_draft_node*> st;
	st.push(m_root_node.get());

	while (!st.empty())
	{
		auto node_ptr = st.top();
		st.pop();
		n += node_ptr->triangles.size();
		if (node_ptr->left) st.push(node_ptr->left.get());
		if (node_ptr->right) st.push(node_ptr->right.get());
	}

	return n;
}

void bvh_draft::build(const scene_cache &cache, const bu::async_stop_flag &stop_flag)
{
	m_root_node = std::make_unique<bu::rt::bvh_draft_node>();
	for (const auto &[id, mesh] : cache.get_meshes())
		if (!mesh->triangles.empty())
			m_root_node->meshes.push_back(mesh);

	process_bvh_node(&stop_flag, m_root_node.get());
}
