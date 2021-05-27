#pragma once
#include <map>
#include <vector>
#include "../../scene.hpp"
#include "bvh.hpp"

namespace bu::rt {

/**
	\brief Corresponds to one scene model node
*/
struct bvh_cache_mesh
{
	bool update_from_model_node(const bu::model_node &node);
	
	glm::mat4 transform;
	std::vector<std::weak_ptr<bu::mesh>> meshes;

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
	bvh_draft build_draft() const;

	const auto &get_meshes() const
	{
		return m_meshes;
	}

	std::vector<rt::aabb> get_mesh_aabbs() const;

private:
	std::map<std::uint64_t, std::shared_ptr<bvh_cache_mesh>> m_meshes;
};

/**
	\brief Node of the BVH tree draft

	Can either contain references to bvh_builder_mesh or triangles.
	dissolve_meshes() transforms referenced meshes to triangles.
*/
struct bvh_draft_node
{
	void dissolve_meshes();
	
	rt::aabb aabb;
	std::unique_ptr<bvh_draft_node> left, right;
	std::vector<std::shared_ptr<bvh_cache_mesh>> meshes;
	std::vector<rt::triangle> triangles;
};

class bvh_draft
{
	friend class bvh_cache;

public:
	std::vector<rt::aabb> get_tree_aabbs() const;

private:
	bvh_draft(const bvh_cache &cache);
	std::unique_ptr<bvh_draft_node> m_root_node;
};





}