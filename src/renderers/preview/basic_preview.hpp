#pragma once
#include <memory>
#include "renderers/basic_gl_renderer.hpp"

namespace bu {

/**
	\todo Free meshes when no one is using the context
*/
struct basic_preview_context : public basic_gl_renderer_context
{
	basic_preview_context();

	std::unique_ptr<bu::shader_program> basic_preview_program;
};

/**
	\brief Provides basic scene preview
*/
class basic_preview_renderer : public bu::basic_gl_renderer
{
public:
	basic_preview_renderer(std::shared_ptr<basic_preview_context> context);

protected:
	void pre_scene_draw(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj) override;
	
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
		const std::shared_ptr<bu::material_data> &material,
		GLsizei draw_size) override;

private:
	std::shared_ptr<basic_preview_context> m_context;
};

}