#include "preview.hpp"
#include <memory>
#include <stack>
#include "../../material.hpp"
#include "../../materials/diffuse_material.hpp"
#include "../../log.hpp"
#include "../../utils.hpp"

using bu::preview_renderer;

preview_renderer::preview_renderer()
{
	{
		auto vs = bu::make_shader(GL_VERTEX_SHADER, bu::slurp_txt("resources/shaders/preview.vs.glsl"));
		auto fs = bu::make_shader(GL_FRAGMENT_SHADER, bu::slurp_txt("resources/shaders/preview.fs.glsl"));
		program = std::make_unique<shader_program>(std::initializer_list<const gl_shader*>{&vs, &fs});
	}

	{
		auto vs = bu::make_shader(GL_VERTEX_SHADER, bu::slurp_txt("resources/shaders/grid.vs.glsl"));
		auto fs = bu::make_shader(GL_FRAGMENT_SHADER, bu::slurp_txt("resources/shaders/grid.fs.glsl"));
		grid_program = std::make_unique<shader_program>(std::initializer_list<const gl_shader*>{&vs, &fs});
	}

	{
		auto vs = bu::make_shader(GL_VERTEX_SHADER, bu::slurp_txt("resources/shaders/outline.vs.glsl"));
		auto gs = bu::make_shader(GL_GEOMETRY_SHADER, bu::slurp_txt("resources/shaders/outline.gs.glsl"));
		auto fs = bu::make_shader(GL_FRAGMENT_SHADER, bu::slurp_txt("resources/shaders/outline.fs.glsl"));
		outline_program = std::make_unique<shader_program>(std::initializer_list<const gl_shader*>{&vs, &gs, &fs});
	}


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
}

/**
	\todo selection and visibility
*/
void preview_renderer::draw(bu::scene &scene, const bu::camera &camera, const std::set<std::shared_ptr<bu::scene_node>> &selection)
{
	const glm::vec3 world_color{0.1, 0.1, 0.1};

	glClearColor(world_color.r, world_color.g, world_color.b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(vao.id());
	glUseProgram(program->id());

	auto mat_view = camera.get_view_matrix();
	auto mat_proj = camera.get_projection_matrix();

	glUniform3fv(program->get_uniform_location("world_color"), 1, &world_color[0]);
	glUniformMatrix4fv(program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);

	for (auto it = scene.root_node->begin(); !(it == scene.root_node->end()); ++it)
	{
		auto node_ptr = &*it;
		auto mesh_node_ptr = dynamic_cast<mesh_node*>(node_ptr);

		// Skip invisible
		if (!node_ptr->is_visible()) continue;

		if (mesh_node_ptr)
		{
			bool is_selected = selection.count(node_ptr->shared_from_this());
			glm::mat4 mat = it.get_transform();

			for (const auto &mesh : mesh_node_ptr->meshes)
			{
				if (!mesh.data) continue;
				auto &mesh_data = *mesh.data;

				if (!mesh_data.gl_buffers)
					mesh_data.buffer();
				
				// Material properties
				glm::vec3 base_color{0.8f};
				float specular_intensity = 0.2f;
				if (mesh.mat)
				{
					if (auto mat = dynamic_cast<const bu::diffuse_material*>(mesh.mat->surface.get()))
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

				glUniform1f(program->get_uniform_location("specular_int"), specular_intensity);
				glUniform3fv(program->get_uniform_location("base_color"), 1, &base_color[0]);
				glUniform1i(program->get_uniform_location("selected"), is_selected);
				glUniformMatrix4fv(program->get_uniform_location("mat_model"), 1, GL_FALSE, &mat[0][0]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data.gl_buffers->index_buffer.id());
				glBindVertexBuffer(0, mesh_data.gl_buffers->vertex_buffer.id(), 0, 8 * sizeof(float));
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data.gl_buffers->index_buffer.id());
				glDrawElements(GL_TRIANGLES, mesh_data.indices.size(), GL_UNSIGNED_INT, nullptr);

				if (is_selected)
				{
					// Do not draw on top of the selected object
					glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 1, 0xff);
					glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

					// Cull faces so the outline doesn't go crazy when the camera is inside the object
					glEnable(GL_CULL_FACE);
					glUseProgram(outline_program->id());
					glUniform1f(outline_program->get_uniform_location("aspect"), camera.aspect);
					glUniformMatrix4fv(outline_program->get_uniform_location("mat_model"), 1, GL_FALSE, &mat[0][0]);
					glUniformMatrix4fv(outline_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
					glUniformMatrix4fv(outline_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
					glDrawElements(GL_TRIANGLES, mesh_data.indices.size(), GL_UNSIGNED_INT, nullptr);
					
					// Go back to normal state
					glUseProgram(program->id());
					glDisable(GL_CULL_FACE);
					glDisable(GL_STENCIL_TEST);
				}
			}
		}
	}

	// Draw grid
	glBindVertexArray(vao_2d.id());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(grid_program->id());
	glUniform3fv(grid_program->get_uniform_location("cam_pos"), 1, &camera.position[0]);
	glUniform3fv(grid_program->get_uniform_location("cam_dir"), 1, &camera.direction[0]);
	glUniform1f(grid_program->get_uniform_location("cam_near"), camera.near);
	glUniform1f(grid_program->get_uniform_location("cam_far"), camera.far);
	glUniformMatrix4fv(grid_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(grid_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}