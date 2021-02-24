#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"
#include "sampled_image.hpp"
#include "worker.hpp"


namespace bu {

struct rt_context
{
	rt_context();

	std::unique_ptr<bu::shader_program> draw_sampled_image;
};

/**
	Path tracing rendering job. The parameters cannot be changed once the job
	has been created.
*/
class rt_renderer_job : public std::enable_shared_from_this<rt_renderer_job>
{
	friend class rt::worker;

public:
	rt_renderer_job(std::shared_ptr<bu::rt_context> context, const bu::camera &camera, const glm::ivec2 &viewport_size);
	~rt_renderer_job();
	
	// Bucket exchange
	void submit_bucket(std::unique_ptr<rt::splat_bucket> bucket);
	std::unique_ptr<rt::splat_bucket> acquire_bucket();

	// Access to the image
	const rt::sampled_image &get_image() const;

	// Updates image
	void update();

	// Run control
	void start();
	void stop();

private:
	void new_buckets(int count, int size);

	// The main context
	std::shared_ptr<bu::rt_context> m_context;

	// Camera ray caster
	bu::camera_ray_caster m_ray_caster;

	// The sampled image
	rt::sampled_image m_image;

	// Buckets
	// \todo Use real atomic vectors here...
	using bucket_pool = std::vector<std::unique_ptr<rt::splat_bucket>>;
	std::mutex m_clean_buckets_mutex;
	bucket_pool m_clean_buckets;
	std::mutex m_dirty_buckets_mutex;
	bucket_pool m_dirty_buckets;

	// Workers - children
	std::vector<std::shared_ptr<rt::worker>> m_workers;
};

/**
	The renderer spawns rt_renderer_job. If any rendering parameters change
	the job is respawned.
*/
class rt_renderer : public bu::renderer
{
public:
	rt_renderer(std::shared_ptr<rt_context> context);
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