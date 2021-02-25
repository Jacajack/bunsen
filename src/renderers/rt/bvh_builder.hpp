#pragma once
#include <map>
#include <vector>
#include "../../scene.hpp"
#include "core.hpp"
#include "bvh.hpp"

namespace bu::rt {

/**
	\brief Corresponds to one scene model node
*/
struct bvh_builder_mesh
{
	bool update_from_model_node(const bu::model_node &node);
	
	glm::mat4 transform;
	std::vector<std::weak_ptr<bu::mesh>> meshes;

	rt::aabb aabb;
	std::vector<rt::triangle> triangles;

	bool visited; //!< Has node been visited in this update_from_scene pass
	bool changed; //!< Has node been considered modified during this update pass
};

/**
	\brief Node of the BVH tree draft

	Can either contain references to bvh_builder_mesh or triangles.
	To transform containes meshes to triangles dissolve_meshes() can be used.
*/
struct bvh_builder_node
{
	void dissolve_meshes();
	
	rt::aabb aabb;
	std::unique_ptr<bvh_builder_node> left, right;
	std::vector<std::weak_ptr<bvh_builder_mesh>> meshes;
	std::vector<rt::triangle> triangles;
};

class bvh_builder
{
public:
	bool update_from_scene(const bu::scene &scene);
	void build_tree();
	bvh_tree make_bvh_tree() const;

	// Debug
	std::vector<rt::aabb> get_mesh_aabbs() const;

private:
	std::map<std::uint64_t, std::shared_ptr<bvh_builder_mesh>> m_meshes;
	std::unique_ptr<bvh_builder_node> m_root_node;
	bool m_scene_changed;
};

}