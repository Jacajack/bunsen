#pragma once
#include <memory>
#include <set>
#include <glm/glm.hpp>
#include "renderers/preview/basic_preview.hpp"

namespace bu {

/**
	\brief Preview renderer's context contains all shaders and VAOs necessary
	for renderer's operation.

	Multiple renderes rendering different scenes can share the same context.
*/
struct preview_context : public basic_preview_context
{
	preview_context();

	std::unique_ptr<bu::shader_program> grid_program;
	std::unique_ptr<bu::shader_program> outline_program;
	std::unique_ptr<bu::shader_program> light_program;
	gl_vertex_array vao_2d;
};

/**
	\brief Renders the scene with simplified materials. Good for editing scene layout.

	Preview renderer can render outlines. Multiple renderer instances
	can share the same context. Context must not be used by concurrent threads, though.
*/
class preview_renderer : public bu::basic_preview_renderer
{
public:
	preview_renderer(std::shared_ptr<preview_context> context);

protected:
	void post_scene_draw(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj) override;

	void draw_light(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj,
		const glm::mat4 &mat_model,
		const bu::light_node &light_node,
		const bool is_selected) override;

	void draw_mesh(
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
		GLsizei draw_size) override;

private:
	std::shared_ptr<preview_context> m_context;
};

}