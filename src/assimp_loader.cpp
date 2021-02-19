#include "assimp_loader.hpp"
#include "log.hpp"
#include "material.hpp"
#include "model.hpp"
#include "materials/diffuse_material.hpp"
#include <stdexcept>
#include <set>
#include <map>

#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

static glm::mat4 assimp_mat4_to_glm(const aiMatrix4x4 &m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

static glm::vec3 assimp_rgb_to_glm(const aiColor3D &c)
{
	return glm::vec3{c.r, c.g, c.b};
}

static std::shared_ptr<bu::material_data> convert_assimp_material(const aiMaterial *am)
{
	auto mat = std::make_shared<bu::material_data>();
	auto surf = std::make_unique<bu::diffuse_material>();

	// Get name
	aiString aname;
	am->Get(AI_MATKEY_NAME, aname);
	mat->name = aname.C_Str();
	LOG_INFO << "Processing material '" << mat->name << "'";

	// Get color
	aiColor3D col;
	am->Get(AI_MATKEY_COLOR_DIFFUSE, col);
	surf->color = assimp_rgb_to_glm(col);

	mat->surface = std::move(surf);
	return mat;
}

/**
	\todo UVs
*/
std::shared_ptr<bu::scene_node> bu::load_mesh_from_file(const std::string &path)
{
	Assimp::Importer importer;

	LOG_INFO << "Importing model '" << path << "'";

	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("load_mesh_from_file() - Assimp import error!");

	// Converts aiMesh to br::mesh_data
	auto process_mesh = [&](aiMesh *m)
	{
		auto mesh_data = std::make_shared<bu::mesh>();
		mesh_data->name = m->mName.C_Str();

		for (auto i = 0u; i < m->mNumVertices; i++)
		{
			mesh_data->positions.emplace_back(
				m->mVertices[i].x,
				m->mVertices[i].y,
				m->mVertices[i].z
			);

			mesh_data->normals.emplace_back(
				m->mNormals[i].x,
				m->mNormals[i].y,
				m->mNormals[i].z
			);

			// TODO UVs
		}

		// Generate indices
		for (auto i = 0u; i < m->mNumFaces; i++)
		{
			const auto &f = m->mFaces[i];
			for (auto j = 0u; j < f.mNumIndices; j++)
				mesh_data->indices.push_back(f.mIndices[j]);
		}

		return mesh_data;
	};

	// Converts assimp node to model
	auto process_model = [&](const aiScene *scene, aiNode *n)
	{
		auto model = std::make_shared<bu::model>();

		std::set<int> assimp_material_ids;

		for (auto i = 0u; i < n->mNumMeshes; i++)
		{
			const auto &assimp_mesh = scene->mMeshes[n->mMeshes[i]];
			auto mesh = process_mesh(assimp_mesh);
			assimp_material_ids.insert(assimp_mesh->mMaterialIndex);
			model->meshes.push_back({mesh, int(assimp_mesh->mMaterialIndex)});
		}

		// Gather all necessary materials
		std::map<int, int> material_aliases;
		for (auto &mesh : model->meshes)
		{
			auto it = material_aliases.find(mesh.material_id);
			if (it == material_aliases.end())
			{
				int n = model->materials.size();
				material_aliases[mesh.material_id] = n;
				model->materials.push_back(convert_assimp_material(scene->mMaterials[mesh.material_id]));
				mesh.material_id = n;
			}
			else
				mesh.material_id = it->second;
		}

		return model;
	};

	// Recursively converts aiNode to bu::scene_node
	auto process_node = [&](const aiScene *scene, aiNode *n)
	{
		auto process_node_impl = [&](const aiScene *scene, aiNode *n, auto &ref)->std::shared_ptr<bu::model_node>
		{
			auto node = std::make_shared<bu::model_node>();
			node->set_name(n->mName.C_Str());
			node->set_transform(assimp_mat4_to_glm(n->mTransformation));
			auto model = process_model(scene, n);
			node->model = model;

			for (auto i = 0u; i < n->mNumChildren; i++)
			{
				// LOG_DEBUG << "adding child to " << n->mName.C_Str() << "(" << node << ")"; 
				node->add_child(ref(scene, n->mChildren[i], ref));
			}

			return node;
		};

		return process_node_impl(scene, n, process_node_impl);
	};

	auto node = process_node(scene, scene->mRootNode);
	
	if (node->get_children().size() == 1 && node->model->get_mesh_count() == 0)
		return node->get_children()[0];
	else
		return node;
}