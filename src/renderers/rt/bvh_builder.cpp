#include "bvh_builder.hpp"
#include <algorithm>
#include <tracy/Tracy.hpp>
#include "../../log.hpp"

using bu::rt::bvh_builder_mesh;
using bu::rt::bvh_builder_node;
using bu::rt::bvh_builder;

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
bool bvh_builder_mesh::update_from_model_node(const bu::model_node &node)
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
		LOG_DEBUG << "BVH mesh change - different mesh number";
	}
	else
	{
		// Detect any missing meshes
		std::set<std::shared_ptr<bu::mesh>> our_meshes;
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
		for (int i = 0; i < model.get_mesh_count(); i++)
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
		for (int i = 0; i < model.get_mesh_count(); i++)
		{
			auto mesh_ptr = model.get_mesh(i);
			meshes.push_back(mesh_ptr);
			auto mesh_tris = mesh_to_triangles(*mesh_ptr, transform);
			std::copy(mesh_tris.begin(), mesh_tris.end(), std::back_insert_iterator(triangles));
		}

		aabb = bu::rt::triangles_aabb(&triangles[0], triangles.size());
	}

	return changed;
}

bool bvh_builder::update_from_scene(const bu::scene &scene)
{
	ZoneScopedN("BVH update from scene");

	bool change = false;

	// Clear 'visited' flag
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
				ptr = std::make_shared<bvh_builder_mesh>();
				LOG_DEBUG << "Adding a new mesh to BVH";
			}
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

	return m_scene_changed = change;
}

std::vector<bu::rt::aabb> bu::rt::bvh_builder::get_mesh_aabbs() const
{
	std::vector<aabb> aabbs;
	aabbs.reserve(m_meshes.size());
	for (const auto &[id, mesh] : m_meshes)
		aabbs.push_back(mesh->aabb);
	return aabbs;
}