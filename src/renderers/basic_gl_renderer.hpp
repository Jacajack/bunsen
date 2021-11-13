#pragma once
#include <unordered_map>
#include <memory>
#include "renderer.hpp"
#include "gl/shader.hpp"
#include "gl/gl.hpp"

namespace bu {

struct basic_gl_renderer_mesh
{
	basic_gl_renderer_mesh(std::shared_ptr<bu::mesh> mesh);

	std::weak_ptr<bu::mesh> source;
	bu::gl_buffer vertex_buffer;
	bu::gl_buffer index_buffer;
	GLsizei size;
};

/**
	\todo Free meshes when no one is using the context
*/
struct basic_gl_renderer_context
{
	basic_gl_renderer_context();

	std::unordered_map<std::uint64_t, basic_gl_renderer_mesh> meshes;
	bu::gl_vertex_array vao;

	basic_gl_renderer_mesh &get_mesh(const std::shared_ptr<bu::mesh> &mesh);
	void unbuffer_outdated_meshes();
};

/**
	\brief Base class for simple OpenGL-based scene forward renderers

	Provides automatical mesh buffering capability in \ref basic_gl_renderer_context.
*/
class basic_gl_renderer : public bu::renderer
{
public:
	basic_gl_renderer(std::shared_ptr<basic_gl_renderer_context> context);
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size) override;

protected:
	virtual void pre_scene_draw(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj)
	{}

	virtual void post_scene_draw(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj)
	{}

	virtual void draw_light(
		const bu::scene &scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		const glm::mat4 &mat_view,
		const glm::mat4 &mat_proj,
		const glm::mat4 &mat_model,
		const bu::light_node &light_node,
		const bool is_selected)
	{}

	/**
		When called, the mesh buffers are already bound to the VAO.
		The mesh is expected to be drawn using:
		`glDrawElements(GL_TRIANGLES, draw_size, GL_UNSIGNED_INT, nullptr);`
	*/
	virtual void draw_mesh(
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
	{}

private:
	std::shared_ptr<basic_gl_renderer_context> m_context;
};

}