#include "basic_gl_renderer.hpp"
#include <vector>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "materials/diffuse_material.hpp"
#include "log.hpp"

using bu::basic_gl_renderer_mesh;
using bu::basic_gl_renderer_context;
using bu::basic_gl_renderer;

basic_gl_renderer_mesh::basic_gl_renderer_mesh(std::shared_ptr<bu::mesh> mesh) :
	source(mesh)
{
	if (mesh->vertices.size() != mesh->normals.size())
	{
		LOG_ERROR << "basic_gl_renderer: Mesh vertex count doesn't match normal count!";
		return;
	}
	
	std::vector<glm::vec3> raw(mesh->vertices.size() + mesh->normals.size());
	for (auto i = 0u; i < mesh->vertices.size(); i++)
	{
		raw[2 * i + 0] = mesh->vertices[i];
		raw[2 * i + 1] = mesh->normals[i];
	}

	glNamedBufferStorage(vertex_buffer.id(), bu::vector_size(raw), raw.data(), GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(index_buffer.id(), bu::vector_size(mesh->indices), mesh->indices.data(), GL_DYNAMIC_STORAGE_BIT);

	size = mesh->indices.size();

	LOG_INFO << "Basic preview renderer finished buffering mesh '" << mesh->name << "'";
}

basic_gl_renderer_context::basic_gl_renderer_context()
{
	glBindVertexArray(vao.id());

	glVertexArrayAttribFormat( // Position (0)
		vao.id(), 
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0
	);

	glVertexArrayAttribFormat( // Normal (1)
		vao.id(), 
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float)
	);

	// Enable attributes
	glEnableVertexArrayAttrib(vao.id(), 0);
	glEnableVertexArrayAttrib(vao.id(), 1);

	// All data is read from buffer bound to binding 0
	glVertexArrayAttribBinding(vao.id(), 0, 0);
	glVertexArrayAttribBinding(vao.id(), 1, 0);

	LOG_INFO << "Created a new basic preview renderer context.";
}

basic_gl_renderer_mesh &basic_gl_renderer_context::get_mesh(const std::shared_ptr<bu::mesh> &mesh)
{
	auto uid = mesh->uid();
	auto it = meshes.find(uid);
	if (it == meshes.end())
	{
		LOG_DEBUG << "basic_gl_renderer buffering mesh '" << mesh->name << "'";
		meshes.emplace(uid, basic_gl_renderer_mesh(mesh));
		return meshes.at(uid);
	}
	else
		return it->second;
}

void basic_gl_renderer_context::unbuffer_outdated_meshes()
{
	for (auto it = meshes.begin(); it != meshes.end(); )
	{
		if (it->second.source.expired())
		{
			LOG_DEBUG << "basic_gl_renderer unbuffering mesh";
			it = meshes.erase(it);
		}
		else
			++it;
	}
}

basic_gl_renderer::basic_gl_renderer(std::shared_ptr<basic_gl_renderer_context> context) :
	m_context(std::move(context))
{
}

void basic_gl_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	ZoneScopedN("basic_gl_renderer::draw");
	TracyGpuZone("basic_gl_renderer::draw");

	auto &ctx = *m_context;

	// Get view and projection matrices
	auto mat_view = camera.get_view_matrix();
	auto mat_proj = camera.get_projection_matrix();

	pre_scene_draw(
		scene,
		camera,
		viewport_size,
		mat_view,
		mat_proj);

	for (auto it = scene.root_node->begin(); !(it == scene.root_node->end()); ++it)
	{
		auto node_ptr = &*it;

		// Skip invisible nodes
		if (!node_ptr->is_visible()) continue;

		bool is_selected = scene.selection.contains(node_ptr->shared_from_this());
		glm::mat4 transform = it.get_transform();

		// Model nodes
		if (auto model_node_ptr = dynamic_cast<bu::model_node*>(node_ptr))
		{
			auto model = model_node_ptr->model;
			if (!model)
			{
				LOG_DEBUG << "Model nodes with no models are present on the scene!";
				continue;
			}

			for (int i = 0; i < model->get_mesh_count(); i++)
			{
				ZoneScopedN("Mesh");
				auto &mesh_buffer = ctx.get_mesh(model->get_mesh(i));
				auto material = model->get_mesh_material(i);

				// Bind buffers
				glBindVertexArray(ctx.vao.id());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_buffer.index_buffer.id());
				glBindVertexBuffer(0, mesh_buffer.vertex_buffer.id(), 0, 6 * sizeof(float));

				draw_mesh(
					scene,
					camera,
					viewport_size,
					mat_view,
					mat_proj,
					transform,
					*model_node_ptr,
					is_selected,
					*model->get_mesh(i),
					material,
					mesh_buffer.size);

				// Unbind buffers to avoid accidentally keeping mesh ownership
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				glBindVertexBuffer(0, 0, 0, 0);
			}
		}

		// Light nodes
		if (auto ln = dynamic_cast<bu::light_node*>(node_ptr))
		{
			draw_light(
				scene,
				camera,
				viewport_size,
				mat_view,
				mat_proj,
				transform,
				*ln,
				is_selected);
		}
	}

	post_scene_draw(
		scene,
		camera,
		viewport_size,
		mat_view,
		mat_proj);
}
