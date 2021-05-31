#pragma once
#include <map>
#include <vector>
#include "../../async_task.hpp"
#include "../../scene.hpp"
#include "aabb.hpp"

namespace bu::rt {
class scene_cache;
struct scene_cache_mesh;

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
	std::vector<std::shared_ptr<const scene_cache_mesh>> meshes;
	std::vector<rt::triangle> triangles;
};

class bvh_draft
{
public:
	void build(const scene_cache &cache, const bu::async_stop_flag &stop_flag);
	std::vector<rt::aabb> get_tree_aabbs() const;
	const bvh_draft_node &get_root_node() const;
	int get_height() const;
	int get_triangle_count() const;

private:
	std::unique_ptr<bvh_draft_node> m_root_node ;
};

}