#include "preview.hpp"
#include <memory>
#include <stack>
#include "../../material.hpp"
#include "../../materials/diffuse_material.hpp"
#include "../../log.hpp"
#include "../../utils.hpp"

using bu::preview_renderer;
using bu::preview_context;

preview_context::preview_context()
{
	program = std::make_unique<shader_program>(bu::load_shader_program("preview"));
	grid_program = std::make_unique<shader_program>(bu::load_shader_program("grid"));
	outline_program = std::make_unique<shader_program>(bu::load_shader_program("outline"));
	light_program = std::make_unique<shader_program>(bu::load_shader_program("bilboard_light"));

	// VAO setup
	glVertexArrayAttribFormat( // Position
		vao.id(), 
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0
	);

	glVertexArrayAttribFormat( // Normal
		vao.id(), 
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float)
	);

	glVertexArrayAttribFormat( // UV
		vao.id(), 
		2,
		2,
		GL_FLOAT,
		GL_FALSE,
		6 * sizeof(float)
	);

	glEnableVertexArrayAttrib(vao.id(), 0);
	glEnableVertexArrayAttrib(vao.id(), 1);
	glEnableVertexArrayAttrib(vao.id(), 2);

	// All data is read from buffer bound to binding 0
	glVertexArrayAttribBinding(vao.id(), 0, 0);
	glVertexArrayAttribBinding(vao.id(), 1, 0);
	glVertexArrayAttribBinding(vao.id(), 2, 0);

	LOG_INFO << "Created a new preview renderer context.";
}

preview_renderer::preview_renderer(std::shared_ptr<preview_context> context) :
	m_context(std::move(context))
{
}

/**
	\todo selection and visibility
*/
void preview_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size)
{
	auto &ctx = *m_context;
	glm::vec3 world_color{0.1, 0.1, 0.1};

	// Get view and projection matrices
	auto mat_view = camera.get_view_matrix();
	auto mat_proj = camera.get_projection_matrix();

	// If world is solid_world, get the color from there
	if (auto w = dynamic_cast<bu::solid_world*>(scene.world.get()))
		world_color = w->color;

	glClearColor(world_color.r, world_color.g, world_color.b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glUseProgram(ctx.program->id());
	glUniform3fv(ctx.program->get_uniform_location("world_color"), 1, &world_color[0]);
	glUniformMatrix4fv(ctx.program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(ctx.program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);

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
				auto mesh_data = model->get_mesh(i);
				auto material = model->get_mesh_material(i);

				// Buffer the mesh if necessary
				if (!mesh_data->gl_buffers)
					mesh_data->buffer();
				
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

				/*
					When an object is selected, a mask of 0x01's is drawn in the
					stencil buffer
				*/
				if (is_selected)
				{
					static const GLint zero = 0;
					glClearBufferiv(GL_STENCIL, 0, &zero);
					glEnable(GL_STENCIL_TEST);
					glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 1, 0xff);
					glStencilOpSeparate(GL_FRONT_AND_BACK, GL_REPLACE, GL_REPLACE, GL_REPLACE);
				}
				else
					glDisable(GL_STENCIL_TEST);

				glBindVertexArray(ctx.vao.id());
				glUseProgram(ctx.program->id());
				glUniform1f(ctx.program->get_uniform_location("specular_int"), specular_intensity);
				glUniform3fv(ctx.program->get_uniform_location("base_color"), 1, &base_color[0]);
				glUniform1i(ctx.program->get_uniform_location("selected"), is_selected);
				glUniformMatrix4fv(ctx.program->get_uniform_location("mat_model"), 1, GL_FALSE, &transform[0][0]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data->gl_buffers->index_buffer.id());
				glBindVertexBuffer(0, mesh_data->gl_buffers->vertex_buffer.id(), 0, 8 * sizeof(float));
				glDrawElements(GL_TRIANGLES, mesh_data->indices.size(), GL_UNSIGNED_INT, nullptr);

				if (is_selected)
				{
					// Do not draw on top of the selected object
					glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 1, 0xff);
					glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

					// Cull faces so the outline doesn't go crazy when the camera is inside the object
					glEnable(GL_CULL_FACE);
					glUseProgram(ctx.outline_program->id());
					glm::vec2 offset = glm::vec2{5} / viewport_size;
					glUniform2fv(ctx.outline_program->get_uniform_location("offset"), 1, &offset[0]);
					glUniformMatrix4fv(ctx.outline_program->get_uniform_location("mat_model"), 1, GL_FALSE, &transform[0][0]);
					glUniformMatrix4fv(ctx.outline_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
					glUniformMatrix4fv(ctx.outline_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
					glDrawElements(GL_TRIANGLES, mesh_data->indices.size(), GL_UNSIGNED_INT, nullptr);
					
					// Go back to normal state
					glUseProgram(ctx.program->id());
					glDisable(GL_CULL_FACE);
					glDisable(GL_STENCIL_TEST);
				}
			}
		}

		// Light nodes
		if (dynamic_cast<bu::light_node*>(node_ptr))
		{
			glm::vec3 color{0.f};
			if (is_selected) color = glm::vec3{0.8f, 0.4f, 0.0};

			glBindVertexArray(ctx.vao_2d.id());
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glUseProgram(ctx.light_program->id());
			glUniformMatrix4fv(ctx.light_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
			glUniformMatrix4fv(ctx.light_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
			glUniform1f(ctx.light_program->get_uniform_location("size"), 0.05f);
			glUniform3fv(ctx.light_program->get_uniform_location("position"), 1, &transform[3][0]);
			glUniform3fv(ctx.light_program->get_uniform_location("color"), 1, &color[0]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisable(GL_BLEND);
		}

	}

	// Draw grid
	glBindVertexArray(ctx.vao_2d.id());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(ctx.grid_program->id());
	glUniform3fv(ctx.grid_program->get_uniform_location("cam_pos"), 1, &camera.position[0]);
	glUniform3fv(ctx.grid_program->get_uniform_location("cam_dir"), 1, &camera.direction[0]);
	glUniform1f(ctx.grid_program->get_uniform_location("cam_near"), camera.near);
	glUniform1f(ctx.grid_program->get_uniform_location("cam_far"), camera.far);
	glUniformMatrix4fv(ctx.grid_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(ctx.grid_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}