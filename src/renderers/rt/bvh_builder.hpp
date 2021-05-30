#pragma once
#include <map>
#include <vector>
#include "../../async_task.hpp"
#include "../../scene.hpp"
#include "aabb.hpp"

namespace bu::rt {

struct bvh_cache_material
{
	std::weak_ptr<bu::material_data> material_data;
	int index;
	bool visited = false;
};

/**
	\brief Corresponds to one scene model node

	\todo MUTEXES!!!!! - This struct can be modified by cache update and read by
	the BVH draft builder at the same time.
*/
struct bvh_cache_mesh
{
	//! \todo Make this member of bvh_cache
	bool update_from_model_node(const bu::model_node &node, const std::map<std::uint64_t, bvh_cache_material> &mat_cache);
	
	glm::mat4 transform;
	std::vector<std::weak_ptr<bu::mesh>> meshes; // Weak pointers to the original meshes

	rt::aabb aabb;
	std::vector<rt::triangle> triangles;

	bool visited; //!< Has node been visited in this update_from_scene pass
	bool changed; //!< Has node been considered modified during current update pass
};

class bvh_draft;

/**
	\brief Caches transformed models for quicker BVH build
*/
class bvh_cache
{
public:
	bool update_from_scene(const bu::scene &scene);
	bvh_draft build_draft(const async_stop_flag &stop_flag) const;

	const auto &get_meshes() const
	{
		return m_meshes;
	}

	std::vector<bu::rt::material> get_materials() const;
	std::vector<rt::aabb> get_mesh_aabbs() const;

private:
	std::map<std::uint64_t, std::shared_ptr<bvh_cache_mesh>> m_meshes;
	std::map<std::uint64_t, bvh_cache_material> m_materials;
};

/**
	\brief Node of the BVH tree draft

	Can either contain references to bvh_builder_mesh or triangles.
	dissolve_meshes() transforms referenced meshes to triangles.
*/
struct bvh_draft_node
{
	void dissolve_meshes();
	int height() const;
	
	rt::aabb aabb;
	std::unique_ptr<bvh_draft_node> left, right;
	std::vector<std::shared_ptr<const bvh_cache_mesh>> meshes;
	std::vector<rt::triangle> triangles;
};

class bvh_draft
{
	friend class bvh_cache;

public:
	std::vector<rt::aabb> get_tree_aabbs() const;
	const bvh_draft_node &get_root_node() const;
	int get_height() const;
	int get_triangle_count() const;

private:
	bvh_draft() = default;
	std::unique_ptr<bvh_draft_node> m_root_node = std::make_unique<bu::rt::bvh_draft_node>();
};

}