#pragma once
#include <vector>
#include <memory>
#include "material.hpp"
#include "mesh.hpp"

namespace bu {

/**
	\brief Models link multiple meshes with materials

	Model owns or shares material and mesh data with other models.
	
	Materials are held in slots and are referenced to with IDs held by the meshes.
	Meshes with invalid material IDs are assumed to have no material assigned.
*/
struct model
{
	struct mesh_material_pair
	{
		std::shared_ptr<bu::mesh> mesh;
		int material_id;
	};

	std::vector<mesh_material_pair> meshes;
	std::vector<std::shared_ptr<bu::material_data>> materials;

	unsigned int get_mesh_count() const
	{
		return meshes.size();
	}

	std::shared_ptr<bu::mesh> get_mesh(int id) const
	{
		return meshes[id].mesh;
	}

	std::shared_ptr<bu::material_data> get_mesh_material(int id) const
	{
		auto mat_id = meshes[id].material_id;
		if (mat_id < 0 || mat_id >= static_cast<int>(materials.size()))
			return {};
		else
			return materials[mat_id];
	}
};

}