#include "bvh_builder.hpp"
#include <algorithm>
#include <future>
#include <tracy/Tracy.hpp>
#include "aabb.hpp"
#include "../../log.hpp"

using bu::rt::bvh_cache_mesh;
using bu::rt::bvh_cache;
using bu::rt::bvh_draft_node;
using bu::rt::bvh_draft;

/**
	\todo materials
*/
std::vector<bu::rt::triangle> mesh_to_triangles(const bu::mesh &mesh, const glm::mat4 &transform)
{
	ZoneScopedN("mesh_to_triangles");

	// Transform matrix for normals
	glm::mat3 tn{transform};
	tn[0] /= glm::dot(tn[0], tn[0]);
	tn[1] /= glm::dot(tn[1], tn[1]);
	tn[2] /= glm::dot(tn[2], tn[2]);

	if (mesh.indices.size() % 3)
		LOG_ERROR << "There are some redundant indices in the mesh!";

	std::vector<bu::rt::triangle> tris(mesh.indices.size() / 3);
	for (auto i = 0u; i < mesh.indices.size(); i++)
	{
		auto ti = i / 3;
		auto vi = i % 3;
		auto index = mesh.indices[i];

		tris[ti].positions[vi] = glm::vec3{transform * glm::vec4{mesh.positions[index], 1}};
		tris[ti].normals[vi] = glm::normalize(tn * mesh.normals[index]);
		
		if (i < mesh.uvs.size())
			tris[ti].uvs[vi] = mesh.uvs[index];
	}

	return tris;
}

/**
	\returns true if the mesh has been updated
*/
bool bvh_cache_mesh::update_from_model_node(const bu::model_node &node)
{
	ZoneScopedN("Update BVH mesh from node");

	// Update transform
	bool transform_changed = false;
	if (transform != node.get_final_transform())
	{
		transform_changed = true;
		transform = node.get_final_transform();
		LOG_DEBUG << "BVH mesh change - transform change";
	}

	// Update meshes
	bool meshes_changed = false;
	auto &model = *node.model;

	// Check if any meshes have been added or removed
	if (meshes.size() != model.get_mesh_count())
	{
		meshes_changed = true;
		LOG_DEBUG << "BVH mesh change - different mesh count";
	}
	else
	{
		// Compare mesh IDs and not pointers for safety
		auto cmp_mesh_ptr = [](std::shared_ptr<bu::mesh> a, std::shared_ptr<bu::mesh> b)
		{
			if (!a) return true;
			else if (!b) return false;
			else return a->uid() < b->uid();
		};

		// Detect any missing meshes
		std::set<std::shared_ptr<bu::mesh>, decltype(cmp_mesh_ptr)> our_meshes(cmp_mesh_ptr);
		for (auto i = 0u; i < meshes.size(); i++)
		{
			auto ptr = meshes[i].lock();
			if (!ptr)
			{
				meshes_changed = true;
				LOG_DEBUG << "BVH mesh change - mesh destructed";
				break;
			}

			our_meshes.emplace(std::move(ptr));
		}

		// Check if the scene node contains the same meshes
		for (auto i = 0u; i < model.get_mesh_count(); i++)
		{
			auto mesh_ptr = model.get_mesh(i);
			auto it = our_meshes.find(mesh_ptr);
			if (it == our_meshes.end())
			{
				meshes_changed = true;
				LOG_DEBUG << "BVH mesh change - different meshes";
				break;
			}
		}
	}

	changed = transform_changed || meshes_changed;

	// Update from the model
	if (changed)
	{
		ZoneScopedN("Actual update from model");
		transform = node.get_final_transform();
		meshes.clear();
		triangles.clear();
		for (auto i = 0u; i < model.get_mesh_count(); i++)
		{
			auto mesh_ptr = model.get_mesh(i);
			meshes.push_back(mesh_ptr);
			auto mesh_tris = mesh_to_triangles(*mesh_ptr, transform);
			std::copy(mesh_tris.begin(), mesh_tris.end(), std::back_insert_iterator(triangles));
		}

		// Handle weird models with no triangles
		if (!triangles.empty())
			aabb = bu::rt::triangles_aabb(&triangles[0], triangles.size());
		else
			aabb = bu::rt::aabb{};
	}

	return changed;
}

void bvh_draft_node::dissolve_meshes()
{
	ZoneScopedN("Dissolve meshes in BVH node");

	for (auto &mesh : meshes)
		std::copy(mesh->triangles.begin(), mesh->triangles.end(), std::back_insert_iterator(triangles));
	meshes.clear();
}

/**
	\brief Updates all cached models and their positions based on the scene, but does
	not rebuild the BVH
*/
bool bvh_cache::update_from_scene(const bu::scene &scene)
{
	ZoneScopedN("BVH update from scene");

	bool change = false;

	// Clear 'visited' flag for all meshes
	for (auto &p : m_meshes)
		p.second->visited = false;

	// Update from scene
	auto &scene_root = *scene.root_node;
	for (bu::scene_node::dfs_iterator it = scene_root.begin(); !(it == scene_root.end()); ++it)
	{
		auto &node = *it;
		if (auto model_node = dynamic_cast<bu::model_node*>(&node))
		{
			auto &ptr = m_meshes[node.uid()];
			if (!ptr)
			{
				ptr = std::make_shared<bvh_cache_mesh>();
				LOG_DEBUG << "Adding a new mesh to BVH";
			}
			LOG_DEBUG << "model_node = " << model_node;
			change |= ptr->update_from_model_node(*model_node);
			ptr->visited = true;
		}
	}

	// Remove all meshes without 'visited' flag
	for (auto it = m_meshes.begin(); it != m_meshes.end();)
	{
		if (!it->second->visited)
		{
			LOG_DEBUG << "Erasing mesh from BVH";
			it = m_meshes.erase(it);
		}
		else
			++it;
	}

	return change;
}

bvh_draft bvh_cache::build_draft() const
{
	return bvh_draft(*this);
}

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
	std::vector<bvh_box> &input, 
	std::vector<unsigned int> &left, 
	std::vector<unsigned int> &right,
	float cost_intersect = 1,
	float cost_traversal = 6)
{
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

		for (auto i = 0u; i < input.size() - 1; i++)
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
	float glm::vec3::* best_axis;
	float best_cost;
	int best_index;
	std::vector<bvh_box> *best_buf;
	sort_in_axis(buf_x, &glm::vec3::x);
	find_best_split(buf_x, cx, ix);
	sort_in_axis(buf_y, &glm::vec3::y);
	find_best_split(buf_y, cy, iy);
	sort_in_axis(buf_z, &glm::vec3::z);
	find_best_split(buf_z, cz, iz);

	if (cx < cy && cx < cz)
	{
		best_axis = &glm::vec3::x;
		best_cost = cx;
		best_index = ix;
		best_buf = &buf_x;
	}
	else if (cy < cx && cy < cz)
	{
		best_axis = &glm::vec3::y;
		best_cost = cy;
		best_index = iy;
		best_buf = &buf_y;
	}
	else
	{
		best_axis = &glm::vec3::z;
		best_cost = cz;
		best_index = iz;
		best_buf = &buf_z;
	}

	// LOG_DEBUG << "best index is " << best_index;
	if (best_index < 0) return false;

	for (int i = 0; i < static_cast<int>(best_buf->size()); i++)
	{
		if (i <= best_index)
			left.push_back((*best_buf)[i].id);
		else
			right.push_back((*best_buf)[i].id);
	}

	return true;
}

/**
	Recursively processes BVH nodes.

	Nodes containing multiple meshes are partitioned and can be split up. If splitting
	is infeasible the, the meshes are dissolved into triangles.

	Nodes containing triangles are recursively partitioned too.
*/
bool process_bvh_node(bu::rt::bvh_draft_node *node, int depth = 0)
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
			auto box = bu::rt::triangle_aabb(node->triangles[i]);
			contents[i] = bvh_box{box.get_center(), box, i};
			
			if (i == 0)
				node->aabb = box;
			else
				node->aabb.add_aabb(box);
		}

		std::vector<unsigned int> l, r;
		bool should_split = partition_boxes_sah(contents, l, r);

		if (should_split)
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
			auto box = node->meshes[i]->aabb;
			contents[i] = bvh_box{box.get_center(), box, i};
			
			if (i == 0)
				node->aabb = node->meshes[i]->aabb;
			else
				node->aabb.add_aabb(node->meshes[i]->aabb);
		}

		std::vector<unsigned int> l, r;
		bool should_split = partition_boxes_sah(contents, l, r);

		if (should_split)
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

		// If no split, dissolve meshes and try this node again
		if (!should_split)
		{
			node->dissolve_meshes();
			return process_bvh_node(node, depth);
		}
	}
	else // Empty node
	{
		return true;
	}

	/*
		Partition recursively and asynchronously on the first 4 levels.
	*/
	if (depth < 4)
	{
		auto async_policy = std::launch::deferred;
		async_policy = std::launch::async;

		std::future<bool> lfut, rfut;
		if (node->left) lfut = std::async(async_policy, process_bvh_node, node->left.get(), depth + 1);
		if (node->right) rfut = std::async(async_policy, process_bvh_node, node->right.get(), depth + 1);
		lfut.wait();
		rfut.wait();
	}
	else
	{
		if (node->left) process_bvh_node(node->left.get(), depth + 1);
		if (node->right) process_bvh_node(node->right.get(), depth + 1);
	}

	return true;
}

std::vector<bu::rt::aabb> bu::rt::bvh_cache::get_mesh_aabbs() const
{
	std::vector<aabb> aabbs;
	aabbs.reserve(m_meshes.size());
	for (const auto &[id, mesh] : m_meshes)
		aabbs.push_back(mesh->aabb);
	return aabbs;
}

bu::rt::bvh_draft::bvh_draft(const bu::rt::bvh_cache &cache) : 
	m_root_node(std::make_unique<bvh_draft_node>())
{
	// Put all meshes in the root node and process it
	for (const auto &[id, mesh] : cache.get_meshes())
		m_root_node->meshes.push_back(mesh);
	process_bvh_node(m_root_node.get());
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