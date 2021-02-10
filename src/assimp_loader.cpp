#include "assimp_loader.hpp"
#include <stdexcept>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

br::mesh br::load_mesh_from_file(const std::string &path)
{
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("load_mesh_from_file() - Assimp import error!");

	br::mesh mesh;

	auto process_mesh = [&](unsigned int base_index, aiMesh *m)
	{
		for (auto i = 0u; i < m->mNumVertices; i++)
		{
			mesh.vertex_positions.emplace_back(
				m->mVertices[i].x,
				m->mVertices[i].y,
				m->mVertices[i].z
			);

			mesh.vertex_normals.emplace_back(
				m->mNormals[i].x,
				m->mNormals[i].y,
				m->mNormals[i].z
			);
		}

		for (auto i = 0u; i < m->mNumFaces; i++)
		{
			const auto &f = m->mFaces[i];
			for (auto j = 0u; j < f.mNumIndices; j++)
				mesh.indices.push_back(base_index + f.mIndices[j]);
		}
	};

	auto process_node = [&](const aiScene *scene, aiNode *node)
	{
		auto process_node_impl = [&](const aiScene *scene, aiNode *node, auto &ref)->void
		{
			for (auto i = 0u; i < node->mNumMeshes; i++)
				process_mesh(mesh.indices.size(), scene->mMeshes[node->mMeshes[i]]);

			for (auto i = 0u; i < node->mNumChildren; i++)
				ref(scene, node->mChildren[i], ref);
		};

		process_node_impl(scene, node, process_node_impl);
	};

	process_node(scene, scene->mRootNode);
	return mesh;
}