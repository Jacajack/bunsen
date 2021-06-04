#include "preview.hpp"
#include <memory>
#include <stack>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "../../material.hpp"
#include "../../materials/diffuse_material.hpp"
#include "../../log.hpp"
#include "../../utils.hpp"

using bu::preview_renderer;
using bu::preview_context;

preview_context::preview_context()
{
	grid_program = std::make_unique<shader_program>(bu::load_shader_program("grid"));
	outline_program = std::make_unique<shader_program>(bu::load_shader_program("outline"));
	light_program = std::make_unique<shader_program>(bu::load_shader_program("bilboard_light"));

	LOG_INFO << "Created a new preview renderer context.";
}

preview_renderer::preview_renderer(std::shared_ptr<preview_context> context) :
	basic_preview_renderer(context),
	m_context(std::move(context))
{
}

void preview_renderer::post_scene_draw(
	const bu::scene &scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	const glm::mat4 &mat_view,
	const glm::mat4 &mat_proj)
{
	// Draw grid
	ZoneNamedN(zgrid, "Grid", true);
	glBindVertexArray(m_context->vao_2d.id());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(m_context->grid_program->id());
	glUniform3fv(m_context->grid_program->get_uniform_location("cam_pos"), 1, &camera.position[0]);
	glUniform3fv(m_context->grid_program->get_uniform_location("cam_dir"), 1, &camera.direction[0]);
	glUniform1f(m_context->grid_program->get_uniform_location("cam_near"), camera.near);
	glUniform1f(m_context->grid_program->get_uniform_location("cam_far"), camera.far);
	glUniformMatrix4fv(m_context->grid_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(m_context->grid_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}

void preview_renderer::draw_light(
	const bu::scene &scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	const glm::mat4 &mat_view,
	const glm::mat4 &mat_proj,
	const glm::mat4 &mat_model,
	const bu::light_node &light_node,
	const bool is_selected)
{
	ZoneScopedN("Light");

	glm::vec2 size = glm::vec2{60} / glm::vec2{viewport_size};
	glm::vec3 color{0.f};
	if (is_selected) color = glm::vec3{0.8f, 0.4f, 0.0};

	glBindVertexArray(m_context->vao_2d.id());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glUseProgram(m_context->light_program->id());
	glUniformMatrix4fv(m_context->light_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(m_context->light_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
	glUniform2fv(m_context->light_program->get_uniform_location("size"), 1, &size[0]);
	glUniform3fv(m_context->light_program->get_uniform_location("position"), 1, &mat_model[3][0]);
	glUniform3fv(m_context->light_program->get_uniform_location("color"), 1, &color[0]);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}

void preview_renderer::draw_mesh(
	const bu::scene &scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	const glm::mat4 &mat_view,
	const glm::mat4 &mat_proj,
	const glm::mat4 &mat_model,
	const bu::model_node &model_node,
	const bool is_selected,
	const bu::mesh &mesh,
	const std::shared_ptr<bu::material_data> &material,
	GLsizei draw_size)
{
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

	basic_preview_renderer::draw_mesh(
		scene,
		camera,
		viewport_size,
		mat_view,
		mat_proj,
		mat_model,
		model_node,
		is_selected,
		mesh,
		material,
		draw_size);

	if (is_selected)
	{
		ZoneScopedN("Outline");

		// Do not draw on top of the selected object
		glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 1, 0xff);
		glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

		// Cull faces so the outline doesn't go crazy when the camera is inside the object
		glEnable(GL_CULL_FACE);
		glUseProgram(m_context->outline_program->id());
		glm::vec2 offset = glm::vec2{5} / glm::vec2{viewport_size};
		glUniform2fv(m_context->outline_program->get_uniform_location("offset"), 1, &offset[0]);
		glUniformMatrix4fv(m_context->outline_program->get_uniform_location("mat_model"), 1, GL_FALSE, &mat_model[0][0]);
		glUniformMatrix4fv(m_context->outline_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
		glUniformMatrix4fv(m_context->outline_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
		glDrawElements(GL_TRIANGLES, draw_size, GL_UNSIGNED_INT, nullptr);
		
		// Go back to normal state
		// glUseProgram(m_context->program->id());
		glDisable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST);
	}
}
