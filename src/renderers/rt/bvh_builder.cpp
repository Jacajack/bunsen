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
		ZoneScopedN("find_best_split()");

		std::vector<glm::vec3> lmin(input.size());
		std::vector<glm::vec3> lmax(input.size());
		std::vector<glm::vec3> rmin(input.size());
		std::vector<glm::vec3> rmax(input.size());

		lmin[0] = input.front().box.min;
		lmax[0] = input.front().box.max;
		for (auto i = 1u; i < input.size(); i++)
		{
			lmin[i] = glm::min(lmin[i - 1], input[i].box.min);
			lmax[i] = glm::max(lmax[i - 1], input[i].box.max);
		}

		rmin[0] = input.back().box.min;
		rmax[0] = input.back().box.max;
		for (auto i = 1u; i < input.size(); i++)
		{
			rmin[i] = glm::min(rmin[i - 1], input[input.size() - i - 1].box.min);
			rmax[i] = glm::max(rmax[i - 1], input[input.size() - i - 1].box.max);
		}

		index = -1;

		if (input.empty()) return;

		// "Leave as is cost" - number of primitives times intersection cost
		float sp = bu::rt::aabb{lmin.back(), lmax.back()}.get_area();
		cost = cost_intersect * lmin.size();

		{
			ZoneScopedN("Best split search");
			for (auto i = 0u; i < input.size() - 1 && !stop_flag.should_stop(); i++)
			{
				float sl = bu::rt::aabb{lmin[i], lmax[i]}.get_area();
				float sr = bu::rt::aabb{rmin[rmin.size() - i - 1], rmax[rmin.size() - i - 1]}.get_area();
				float c = cost_traversal + cost_intersect / sp * (sl * (i + 1) + sr * (rmin.size() - (i + 1)));

				// LOG_DEBUG << "split " << i << " sl=" << sl << " sr=" << sr << " c=" << c << " best=" << cost;

				if (c < cost)
				{
					cost = c;
					index = i;
				}
			}
		}

		// LOG_DEBUG << "best cost " << cost << " index " << index;
	};

	std::vector<bvh_box> buf_x, buf_y, buf_z;
	float cx, cy, cz;
	int ix, iy, iz;
	int best_index{};
	std::vector<bvh_box> *best_buf{};

	auto split_x = [
		sort_in_axis,
		find_best_split,
		&stop_flag, input,
		&buf_x, &cx, &ix]()
	{
		ZoneScopedN("X split");
		buf_x = input;
		sort_in_axis(buf_x, &glm::vec3::x);
		if (stop_flag.should_stop()) return;
		find_best_split(buf_x, cx, ix);
	};

	auto split_y = [
		sort_in_axis,
		find_best_split,
		&stop_flag, input,
		&buf_y, &cy, &iy]
	{
		ZoneScopedN("Y split");
		buf_y = input;
		sort_in_axis(buf_y, &glm::vec3::y);
		if (stop_flag.should_stop()) return;
		find_best_split(buf_y, cy, iy);
	};

	auto split_z = [
		sort_in_axis,
		find_best_split,
		&stop_flag, input,
		&buf_z, &cz, &iz]
	{
		ZoneScopedN("Z split");
		buf_z = input;
		sort_in_axis(buf_z, &glm::vec3::z);
		if (stop_flag.should_stop()) return;
		find_best_split(buf_z, cz, iz);
	};

	// Depending on number of items to partition
	// find splits in parallel or sequentially
	if (input.size() > 10000)
	{
		auto fut_x = std::async(std::launch::async, split_x);
		auto fut_y = std::async(std::launch::async, split_y);
		split_z();
		fut_x.wait();
		fut_y.wait();
	}
	else
	{
		split_x();
		if (stop_flag.should_stop()) return false;
		split_y();
		if (stop_flag.should_stop()) return false;
		split_z();
	}

	if (stop_flag.should_stop()) return false;

	if (cx < cy && cx < cz)
	{
		best_index = ix;
		best_buf = &buf_x;
		buf_y.clear();
		buf_y.shrink_to_fit();
		buf_z.clear();
		buf_z.shrink_to_fit();
	}
	else if (cy < cx && cy < cz)
	{
		best_index = iy;
		best_buf = &buf_y;
		buf_x.clear();
		buf_x.shrink_to_fit();
		buf_z.clear();
		buf_z.shrink_to_fit();
	}
	else
	{
		best_index = iz;
		best_buf = &buf_z;
		buf_x.clear();
		buf_x.shrink_to_fit();
		buf_y.clear();
		buf_y.shrink_to_fit();
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

static void partition_triangles(const bu::async_stop_flag *stop_flag, bu::rt::bvh_draft_node *node)
{
	ZoneScopedN("BVH triangle partitioning");

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
		node->triangles.shrink_to_fit();
	}
}

/**
	\returns true if the the meshes in this node should be dissolved
*/
static bool partition_meshes(const bu::async_stop_flag *stop_flag, bu::rt::bvh_draft_node *node)
{
	ZoneScopedN("BVH mesh partitioning");

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
		node->meshes.shrink_to_fit();
	}

	return !should_split || !split_valid;
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
	ZoneScopedN("BVH node processing");
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
		partition_triangles(stop_flag, node);
	}
	else if (!node->meshes.empty()) // Has meshes - try to partition them
	{
		// If splitting fails, dissolve meshes
		if (partition_meshes(stop_flag, node))
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

		std::future<bool> lfut;
		if (node->left) lfut = std::async(async_policy, process_bvh_node, stop_flag, node->left.get(), depth + 1);
		if (node->right) process_bvh_node(stop_flag, node->right.get(), depth + 1);
		if (node->left) lfut.wait();
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
	ZoneScopedN("BVH meshes dissolve");
	for (auto &mesh : meshes)
		std::copy(mesh->triangles.begin(), mesh->triangles.end(), std::back_insert_iterator(triangles));
	meshes.clear();
	meshes.shrink_to_fit();
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
	ZoneScopedN("BVH draft build");

	m_root_node = std::make_unique<bu::rt::bvh_draft_node>();
	for (const auto &[id, mesh] : cache.get_meshes())
		if (mesh->visible && !mesh->triangles.empty())
			m_root_node->meshes.push_back(mesh);

	process_bvh_node(&stop_flag, m_root_node.get());
}
