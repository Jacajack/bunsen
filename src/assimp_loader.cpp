#include "assimp_loader.hpp"
#include "log.hpp"
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

glm::mat4 assimp_mat4_to_glm(const aiMatrix4x4 &m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

bu::scene_node bu::load_mesh_from_file(const std::string &path)
{
	Assimp::Importer importer;

	LOG_INFO << "Importing model '" << path << "'";

	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("load_mesh_from_file() - Assimp import error!");

	// Converts aiMesh to br::mesh_data
	auto process_mesh = [&](aiMesh *m)
	{
		auto mesh_data = std::make_shared<bu::mesh_data>();
		mesh_data->name = m->mName.C_Str();
		LOG_DEBUG << "Processing mesh '" << mesh_data->name << "'";

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

	// Recursively converts aiNode to br::scene_node
	auto process_node = [&](const aiScene *scene, aiNode *n)
	{
		auto process_node_impl = [&](const aiScene *scene, aiNode *n, auto &ref)->bu::scene_node
		{
			bu::scene_node node;
			node.name = n->mName.C_Str();
			node.transform = assimp_mat4_to_glm(n->mTransformation);
			LOG_DEBUG << "Processing node '" << node.name << "'";

			for (auto i = 0u; i < n->mNumMeshes; i++)
			{
				auto mesh_data = process_mesh(scene->mMeshes[n->mMeshes[i]]);
				node.meshes.push_back(bu::mesh{mesh_data, nullptr});
			}

			for (auto i = 0u; i < n->mNumChildren; i++)
				node.children.emplace_back(ref(scene, n->mChildren[i], ref));

			return node;
		};

		return process_node_impl(scene, n, process_node_impl);
	};

	return process_node(scene, scene->mRootNode);
}