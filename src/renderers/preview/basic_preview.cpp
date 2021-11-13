#include "basic_preview.hpp"
#include <vector>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "../../materials/diffuse_material.hpp"
#include "../../log.hpp"

using bu::basic_preview_context;
using bu::basic_preview_renderer;

basic_preview_context::basic_preview_context() :
	basic_preview_program(std::make_unique<bu::shader_program>(bu::load_shader_program("basic_preview")))
{
	LOG_INFO << "Created a new basic preview renderer context.";
}

basic_preview_renderer::basic_preview_renderer(std::shared_ptr<basic_preview_context> context) :
	basic_gl_renderer(context),
	m_context(std::move(context))
{
}

void basic_preview_renderer::pre_scene_draw(
	const bu::scene &scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	const glm::mat4 &mat_view,
	const glm::mat4 &mat_proj)
{
	// If world is solid_world, get the color from there
	glm::vec3 world_color{0.1, 0.1, 0.1};
	if (auto w = dynamic_cast<bu::solid_world*>(scene.world.get()))
		world_color = w->color;

	glClearColor(world_color.r, world_color.g, world_color.b, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glUseProgram(m_context->basic_preview_program->id());
	glUniform3fv(m_context->basic_preview_program->get_uniform_location("world_color"), 1, &world_color[0]);
	glUniformMatrix4fv(m_context->basic_preview_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
	glUniformMatrix4fv(m_context->basic_preview_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
}

void basic_preview_renderer::draw_mesh(
	const bu::scene &scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	const glm::mat4 &mat_view,
	const glm::mat4 &mat_proj,
	const glm::mat4 &mat_model,
	const bu::model_node &model_node,
	const bool is_selected,
	const bu::mesh &mesh,
	const bu::resource_handle<bu::material_resource> &material,
	GLsizei draw_size)
{
	// Material properties
	glm::vec3 base_color{0.8f};
	float specular_intensity = 0.2f;
	if (auto mat = dynamic_cast<const bu::diffuse_material*>(material->r()->surface.get()))
	{
		base_color = mat->color;
		specular_intensity = 0.2;
	}

	glUseProgram(m_context->basic_preview_program->id());
	glUniform1f(m_context->basic_preview_program->get_uniform_location("specular_int"), specular_intensity);
	glUniform3fv(m_context->basic_preview_program->get_uniform_location("base_color"), 1, &base_color[0]);
	glUniformMatrix4fv(m_context->basic_preview_program->get_uniform_location("mat_model"), 1, GL_FALSE, &mat_model[0][0]);
	glDrawElements(GL_TRIANGLES, draw_size, GL_UNSIGNED_INT, nullptr);
}
