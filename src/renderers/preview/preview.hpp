#pragma once
#include <memory>
#include <set>
#include <glm/glm.hpp>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"

namespace bu {

/**
	\brief Preview renderer's context contains all shaders and VAOs necessary
	for renderer's operation.

	Multiple renderes rendering different scenes can share the same context. The
	context does not currently store any scene-related information. This, however
	might change when renderers will become responsible for buffering meshes.
*/
struct preview_context
{
	preview_context();

	std::unique_ptr<bu::shader_program> program;
	std::unique_ptr<bu::shader_program> grid_program;
	std::unique_ptr<bu::shader_program> outline_program;
	std::unique_ptr<bu::shader_program> light_program;
	gl_vertex_array vao;
	gl_vertex_array vao_2d;
};

/**
	\brief Renders the scene with simplified materials. Good for editing scene layout.

	Preview renderer can render outlines. Multiple renderer instances
	can share the same context. Context must not be used by concurrent threads, though.
*/
class preview_renderer : public bu::renderer
{
public:
	preview_renderer(std::shared_ptr<preview_context> context);
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size) override;

private:
	std::shared_ptr<preview_context> m_context;
};

}