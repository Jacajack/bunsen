#include "scene_cache.hpp"
#include <tracy/Tracy.hpp>
#include "aabb.hpp"
#include "material.hpp"
#include "../../mesh.hpp"
#include "../../model.hpp"
#include "../../material.hpp"
#include "../../log.hpp"
#include "bvh_builder.hpp"

using bu::rt::scene_cache;
using bu::rt::scene_cache_material;
using bu::rt::scene_cache_mesh;

static std::vector<bu::rt::triangle> mesh_to_triangles(const bu::mesh &mesh, const glm::mat4 &transform, int material_id)
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

		tris[ti].material_id = material_id;
		tris[ti].vertices[vi] = glm::vec3{transform * glm::vec4{mesh.vertices[index], 1}};
		tris[ti].normals[vi] = glm::normalize(tn * mesh.normals[index]);
		
		if (i < mesh.uvs.size())
			tris[ti].uvs[vi] = mesh.uvs[index];
	}

	return tris;
}

/**
	\returns true if the mesh has been updated
*/
bool scene_cache::update_from_model_node(
	bu::rt::scene_cache_mesh &cached_mesh,
	const bu::model_node &node,
	bool force_update)
{
	ZoneScopedN("Update BVH mesh from node");

	// Update transform
	bool transform_changed = false;
	if (cached_mesh.transform != node.get_final_transform())
	{
		transform_changed = true;
		cached_mesh.transform = node.get_final_transform();
		// LOG_DEBUG << "BVH mesh change - transform change";
	}

	// Update meshes
	bool meshes_changed = false;
	auto &model = *node.model;

	// Check if any meshes have been added or removed
	if (cached_mesh.meshes.size() != model.get_mesh_count())
	{
		meshes_changed = true;
		// LOG_DEBUG << "BVH mesh change - different mesh count";
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
		for (const auto &wptr : cached_mesh.meshes)
		{
			auto ptr = wptr.lock();
			if (!wptr.lock())
			{
				meshes_changed = true;
				// LOG_DEBUG << "BVH mesh change - mesh destructed";
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
				// LOG_DEBUG << "BVH mesh change - different meshes";
				break;
			}
		}
	}

	bool changed = transform_changed || meshes_changed || force_update;

	// Update from the model
	if (changed)
	{
		ZoneScopedN("Actual update from model");
		cached_mesh.transform = node.get_final_transform();
		cached_mesh.meshes.clear();
		cached_mesh.triangles.clear();
		for (auto i = 0u; i < model.get_mesh_count(); i++)
		{
			auto mesh_ptr = model.get_mesh(i);
			cached_mesh.meshes.push_back(mesh_ptr);
			auto mesh_tris = mesh_to_triangles(*mesh_ptr, cached_mesh.transform, m_materials.at(model.get_mesh_material(i)->uid()).index);
			std::copy(mesh_tris.begin(), mesh_tris.end(), std::back_insert_iterator(cached_mesh.triangles));
		}

		cached_mesh.aabb = bu::rt::triangles_aabb(cached_mesh.triangles.data(), cached_mesh.triangles.size());
	}

	return changed;
}

/**
	\returns a pair of booleans. The first one indicated whether any material in the
	cache has been updated (the rendering should be restarted). The second indicates
	whether the material cache has been updated in a way such that the mesh cache has
	to be rebuilt as well.
*/
std::pair<bool, bool> scene_cache::update_materials(const bu::scene &scene)
{
	auto &scene_root = *scene.root_node;
	bool changed = false;
	bool full_rebuild = false;

	// Mark all cached materials as unvisited
	int visited_materials = 0;
	for (auto &p : m_materials)
		p.second.visited = false;

	// Visit all materials in the scene
	for (bu::scene_node::dfs_iterator it = scene_root.begin(); !(it == scene_root.end()); ++it)
	{
		auto &node = *it;
		if (auto model_node = dynamic_cast<bu::model_node*>(&node))
		{
			for (auto mat_ptr : model_node->model->materials)
			{
				auto it = m_materials.find(mat_ptr->uid());
				if (it != m_materials.end())
				{
					auto &cached_mat = it->second;
					cached_mat.visited = true;
					visited_materials++;
				}
			}
		}
	}

	// If there are more than max_unused_materials, rebuild material cache completely
	const int max_unused_materials = 64;
	if (m_materials.size() - visited_materials > max_unused_materials)
	{
		m_materials.clear();
		full_rebuild = true;
	}

	// Update material pointers from scene
	for (bu::scene_node::dfs_iterator it = scene_root.begin(); !(it == scene_root.end()); ++it)
	{
		auto &node = *it;
		if (auto model_node = dynamic_cast<bu::model_node*>(&node))
		{
			for (auto mat_ptr : model_node->model->materials)
			{
				auto it = m_materials.find(mat_ptr->uid());
				if (it == m_materials.end())
				{
					auto &cached_mat = m_materials[mat_ptr->uid()];
					cached_mat.material_data = mat_ptr;
					cached_mat.index = m_materials.size() - 1;
				}
				else
				{
					auto &cached_mat = it->second;
					if (cached_mat.material_data.lock() != mat_ptr)
						changed = true;
					cached_mat.material_data = mat_ptr;
				}
			}
		}
	}

	return {changed, full_rebuild};
}

bool scene_cache::update_meshes(const bu::scene &scene, bool force_update)
{
	auto &scene_root = *scene.root_node;
	bool changed = false;

	// Clear 'visited' flag for all meshes and materials
	for (auto &p : m_meshes)
		p.second->visited = false;

	// Update from scene
	for (bu::scene_node::dfs_iterator it = scene_root.begin(); !(it == scene_root.end()); ++it)
	{
		auto &node = *it;
		if (auto model_node = dynamic_cast<bu::model_node*>(&node))
		{
			auto &ptr = m_meshes[node.uid()];
			if (!ptr)
			{
				ptr = std::make_shared<scene_cache_mesh>();
				LOG_DEBUG << "Adding a new mesh to BVH";
			}
			changed |= update_from_model_node(*ptr, *model_node, force_update);
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

	return changed;
}

/**
	\returns a pair of boolean. The first one indicates whether the scene should
	be updates. The second indicates whether the BVH has to be rebuilt.
*/
std::pair<bool, bool> scene_cache::update_from_scene(const bu::scene &scene)
{
	ZoneScopedN("RT cache update from scene");

	auto [materials_changed, force_mesh_update] = update_materials(scene);
	bool meshes_changed = update_meshes(scene, force_mesh_update);
	return {materials_changed, meshes_changed};
}

std::vector<bu::rt::material> bu::rt::scene_cache::get_materials() const
{
	std::vector<bu::rt::material> materials(m_materials.size());
	for (const auto &[uid, mat] : m_materials)
		materials.at(mat.index) = bu::rt::material{*mat.material_data.lock()};
	return materials;
}

std::vector<bu::rt::aabb> bu::rt::scene_cache::get_mesh_aabbs() const
{
	std::vector<aabb> aabbs;
	aabbs.reserve(m_meshes.size());
	for (const auto &[id, mesh] : m_meshes)
		aabbs.push_back(mesh->aabb);
	return aabbs;
}
