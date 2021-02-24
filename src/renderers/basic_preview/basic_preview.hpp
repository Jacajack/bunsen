#pragma once
#include <unordered_map>
#include <memory>
#include "../../renderer.hpp"
#include "../../gl/shader.hpp"
#include "../../gl/gl.hpp"

namespace bu {

struct basic_preview_mesh
{
	basic_preview_mesh(std::shared_ptr<bu::mesh> mesh);

	std::weak_ptr<bu::mesh> source;
	bu::gl_buffer vertex_buffer;
	bu::gl_buffer index_buffer;
	GLsizei size;
};

/**
	\todo Free meshes when no one is using the context
*/
struct basic_preview_context
{
	basic_preview_context();

	std::unordered_map<std::uint64_t, basic_preview_mesh> meshes;
	std::unique_ptr<bu::shader_program> basic_preview_program;
	bu::gl_vertex_array vao;

	basic_preview_mesh &get_mesh(const std::shared_ptr<bu::mesh> &mesh);
};

/**
	\brief Provides basic scene preview

	Selection and grid are not drawn.

	More than anything else, this is a test of buffering meshes by the renderer
	context.
*/
class basic_preview_renderer : public bu::renderer
{
public:
	basic_preview_renderer(std::shared_ptr<basic_preview_context> context);
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size) override;

private:
	std::shared_ptr<basic_preview_context> m_context;
};

}