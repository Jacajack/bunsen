#pragma once
#include <vector>
#include <memory>
#include "material.hpp"
#include "mesh.hpp"
#include "modified_flag.hpp"

namespace bu {

/**
	\brief Models link multiple meshes with materials

	Model owns or shares material and mesh data with other models.
	
	Materials are held in slots and are referenced to with IDs held by the meshes.
	Meshes with invalid material IDs are assumed to have no material assigned.

	\todo Refactor this, I don't like this mesh_mat things
*/
struct model final : public modified_flag
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

	/**
		\todo fixme
	*/
	void clear_modified() override
	{
		for (auto &mesh_mat : meshes)
		{
			// mesh_mat.mesh->clear_modified(); //!< \todo FIXME
			get_mesh_material(mesh_mat.material_id)->clear_modified();
		}
	}

	/**
		\todo fixme
	*/
	bool is_modified() const override 
	{
		bool mod = false;
		for (auto mesh_mat : meshes)
		{
			// mod |= mesh_mat.mesh->is_modified();
			mod |= get_mesh_material(mesh_mat.material_id)->is_modified();
		}
		return mod;
	}
};

}