#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <chrono>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"
#include "sampled_image.hpp"

namespace bu {

struct rt_renderer_job;

struct rt_context
{
	rt_context();

	std::unique_ptr<bu::shader_program> draw_sampled_image;
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
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size) override;

private:
	void new_texture_storage(const glm::ivec2 &size);
	void set_viewport_size(const glm::ivec2 &viewport_size);

	std::shared_ptr<rt_context> m_context;

	// The output texture, its size and a PBO
	std::unique_ptr<bu::gl_texture> m_result_tex;
	bu::gl_buffer m_pbo;
	glm::ivec2 m_result_tex_size;

	// Current settings
	// Changing these will restart the job
	glm::ivec2 m_viewport;
	bu::camera m_camera;
	std::chrono::time_point<std::chrono::steady_clock> m_last_change;
	bool m_active = false;

	// Current job
	std::shared_ptr<rt_renderer_job> m_job;
};

}