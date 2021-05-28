#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <chrono>
#include "../../async_task.hpp"
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"
#include "../../renderers/basic_preview/basic_preview.hpp"

namespace bu::rt {
class bvh_cache;
class bvh_draft;
struct bvh_tree;
}

namespace bu {
struct rt_renderer_job;

struct rt_context
{
	rt_context(std::shared_ptr<bu::basic_preview_context> preview_ctx = {});

	// Embedded preview renderer
	std::shared_ptr<bu::basic_preview_context> preview_context;

	// Shader for drawing the raytraced image and AABBs
	std::unique_ptr<bu::shader_program> draw_sampled_image;
	std::unique_ptr<bu::shader_program> draw_aabb;

	// VAO for drawing AABBs
	bu::gl_vertex_array aabb_vao;

	// BVH
	std::shared_ptr<bu::rt::bvh_cache> bvh_cache;
	std::shared_ptr<bu::rt::bvh_draft> bvh_draft;
	std::shared_ptr<bu::rt::bvh_tree> bvh;

	std::optional<bu::async_task<std::unique_ptr<bu::rt::bvh_draft>>> bvh_draft_build_task;
	std::optional<bu::async_task<std::unique_ptr<bu::rt::bvh_tree>>> bvh_build_task;

	void update_bvh(const bu::scene &scene, bool rebuild);
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

	// AABB preview buffer
	bool m_preview_active = true;
	bu::gl_buffer aabb_buffer;

	// The output texture, its size and a PBO
	std::unique_ptr<bu::gl_texture> m_result_tex;
	bu::gl_buffer m_pbo;
	glm::ivec2 m_result_tex_size;

	// Current settings
	// Changing these will restart the job
	glm::ivec2 m_viewport;
	bu::camera m_camera;
	std::chrono::time_point<std::chrono::steady_clock> m_last_change;
	const bu::rt::bvh_tree *m_last_bvh;
	bool m_active = false;

	// Current job
	std::unique_ptr<rt_renderer_job> m_job;
};

}