#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <chrono>
#include "async_task.hpp"
#include "gl/shader.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include "renderer.hpp"
#include "renderers/preview/basic_preview.hpp"
#include "event.hpp"

namespace bu::rt {
class scene_cache;
class bvh_draft;
struct bvh_tree;
struct material;
struct scene;
}

namespace bu {
class rt_renderer_job;

class rt_context
{
public:
	rt_context(bu::event_bus &bus, std::shared_ptr<bu::basic_preview_context> preview_ctx = {});

	void update_from_scene(const bu::scene &scene, bool allow_rebuild);

	auto get_basic_preview_context() const {return m_preview_context;}
	auto &get_sampled_image_program() const {return *m_sampled_image_program;}
	auto &get_aabb_program() const {return *m_aabb_program;}

	auto &get_2d_vao() const {return m_2d_vao;}

	auto get_aabb_count() const {return m_aabb_count;}
	auto &get_aabb_buffer() const {return m_aabb_buffer;}
	auto &get_aabb_vao() const {return m_aabb_vao;}

	auto get_scene() const {return m_scene;}
	auto get_scene_cache() const {return m_scene_cache;}

private:
	// Event bus connection
	std::shared_ptr<bu::event_bus_connection> m_events;

	// Embedded preview renderer
	std::shared_ptr<bu::basic_preview_context> m_preview_context;

	// Shader for drawing the raytraced image and AABBs
	std::unique_ptr<bu::shader_program> m_sampled_image_program;
	std::unique_ptr<bu::shader_program> m_aabb_program;

	// VAO for 2D drawing
	bu::gl_vertex_array m_2d_vao;

	// AABB data for drawing BVH preview
	bu::gl_vertex_array m_aabb_vao;
	bu::gl_buffer m_aabb_buffer;
	int m_aabb_count = 0;

	// Scene cache and scene
	std::shared_ptr<bu::rt::scene_cache> m_scene_cache;
	std::shared_ptr<bu::rt::scene> m_scene;

	std::optional<bu::async_task<std::unique_ptr<bu::rt::bvh_draft>>> m_bvh_draft_build_task;
	std::optional<bu::async_task<std::unique_ptr<bu::rt::scene>>> m_scene_build_task;
};

/**
	The renderer spawns rt_renderer_job. If any rendering parameters change
	the job is respawned.
*/
class rt_renderer : public bu::renderer
{
public:
	rt_renderer(std::shared_ptr<rt_context> context);
	~rt_renderer() override;
	void update() override;
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size) override;

private:
	void new_texture_storage(const glm::ivec2 &size);
	void set_viewport_size(const glm::ivec2 &viewport_size);

	std::shared_ptr<rt_context> m_context;
	std::unique_ptr<bu::basic_preview_renderer> m_preview_renderer;

	bool m_preview_active = true;

	// The output texture, its size and a PBO
	std::unique_ptr<bu::gl_texture> m_result_tex;
	bu::gl_buffer m_pbo;
	glm::ivec2 m_result_tex_size;

	// Current settings
	// Changing these will restart the job
	glm::ivec2 m_viewport;
	bu::camera m_camera;
	std::chrono::time_point<std::chrono::steady_clock> m_last_change;
	const bu::rt::scene *m_last_scene;
	bool m_active = false;

	// Current job
	std::unique_ptr<rt_renderer_job> m_job;
};

}