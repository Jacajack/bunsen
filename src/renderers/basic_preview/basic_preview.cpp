#include "basic_preview.hpp"
#include <vector>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "../../materials/diffuse_material.hpp"
#include "../../log.hpp"

using bu::basic_preview_mesh;
using bu::basic_preview_context;
using bu::basic_preview_renderer;

basic_preview_mesh::basic_preview_mesh(std::shared_ptr<bu::mesh> mesh)
{
	if (mesh->positions.size() != mesh->normals.size())
	{
		LOG_ERROR << "basic_preview_renderer: Mesh vertex count doesn't match normal count!";
		return;
	}
	
	std::vector<glm::vec3> raw(mesh->positions.size() + mesh->normals.size());
	for (auto i = 0u; i < mesh->positions.size(); i++)
	{
		raw[2 * i + 0] = mesh->positions[i];
		raw[2 * i + 1] = mesh->normals[i];
	}

	// glBindBuffer(GL_VERTEX_ARRAY, vertex_buffer.id());
	// glBufferStorage(GL_VERTEX_ARRAY, bu::vector_size(raw), raw.data(), GL_STATIC_DRAW);

	// glBindBuffer(GL_INDEX_ARRAY, index_buffer.id());
	// glBufferStorage(GL_INDEX_ARRAY, bu::vector_size(mesh->indices), mesh->indices.data(), GL_STATIC_DRAW);

	glNamedBufferStorage(vertex_buffer.id(), bu::vector_size(raw), raw.data(), GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(index_buffer.id(), bu::vector_size(mesh->indices), mesh->indices.data(), GL_DYNAMIC_STORAGE_BIT);

	size = mesh->indices.size();

	LOG_INFO << "Basic preview renderer finished buffering mesh '" << mesh->name << "'";
}

basic_preview_context::basic_preview_context() :
	basic_preview_program(std::make_unique<bu::shader_program>(bu::load_shader_program("basic_preview")))
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

basic_preview_mesh &basic_preview_context::get_mesh(const std::shared_ptr<bu::mesh> &mesh)
{
	auto uid = mesh->uid();
	auto it = meshes.find(uid);
	if (it == meshes.end())
	{
		meshes.emplace(uid, basic_preview_mesh(mesh));
		return meshes.at(uid);
	}
	else
		return it->second;
}

basic_preview_renderer::basic_preview_renderer(std::shared_ptr<basic_preview_context> context) :
	m_context(std::move(context))
{
}

void basic_preview_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	ZoneScopedN("basic_preview_renderer::draw");
	TracyGpuZone("Basic preview");

	auto &ctx = *m_context;

	// Get view and projection matrices
	auto mat_view = camera.get_view_matrix();
	auto mat_proj = camera.get_projection_matrix();

	// If world is solid_world, get the color from there
	glm::vec3 world_color{0.1, 0.1, 0.1};
	if (auto w = dynamic_cast<bu::solid_world*>(scene.world.get()))
		world_color = w->color;

	glClearColor(world_color.r, world_color.g, world_color.b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glUseProgram(ctx.basic_preview_program->id());
	glUniform3fv(ctx.basic_preview_program->get_uniform_location("world_color"), 1, &world_color[0]);
	glUniformMatrix4fv(ctx.basic_preview_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(ctx.basic_preview_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);

	for (auto it = scene.root_node->begin(); !(it == scene.root_node->end()); ++it)
	{
		auto node_ptr = &*it;

		// Skip invisible
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

				
				// Material properties
				glm::vec3 base_color{0.8f};
				float specular_intensity = 0.2f;
				if (material)
				{
					if (auto mat = dynamic_cast<const bu::diffuse_material*>(material->surface.get()))
					{
						base_color = mat->color;
						specular_intensity = 0.2;
					}
				}

				glBindVertexArray(ctx.vao.id());
				glUseProgram(ctx.basic_preview_program->id());
				glUniform1f(ctx.basic_preview_program->get_uniform_location("specular_int"), specular_intensity);
				glUniform3fv(ctx.basic_preview_program->get_uniform_location("base_color"), 1, &base_color[0]);
				glUniformMatrix4fv(ctx.basic_preview_program->get_uniform_location("mat_model"), 1, GL_FALSE, &transform[0][0]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_buffer.index_buffer.id());
				glBindVertexBuffer(0, mesh_buffer.vertex_buffer.id(), 0, 6 * sizeof(float));
				glDrawElements(GL_TRIANGLES, mesh_buffer.size, GL_UNSIGNED_INT, nullptr);
			}
		}
	}
}
