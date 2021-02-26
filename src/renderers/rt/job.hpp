#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <future>
#include <thread>
#include "../../camera.hpp"
#include "sampled_image.hpp"

namespace bu {

struct rt_context;

struct splat_bucket_pool
{
	std::mutex mut;
	std::condition_variable cv;
	std::vector<std::unique_ptr<rt::splat_bucket>> buckets;

	void submit(std::unique_ptr<rt::splat_bucket> bucket);
	std::unique_ptr<rt::splat_bucket> acquire();
};

/**
	Path tracing rendering job.
*/
class rt_renderer_job : public std::enable_shared_from_this<rt_renderer_job>
{
public:
	rt_renderer_job(std::shared_ptr<bu::rt_context> context);
	~rt_renderer_job();
	
	// Access to the image
	const rt::sampled_image &get_image() const;

	// Run control
	void start(const bu::camera &camera, const glm::ivec2 &viewport_size);
	void stop();

	// View settings
	std::shared_ptr<bu::camera_ray_caster> m_ray_caster;

	// The sampled image
	std::shared_ptr<rt::sampled_image> m_image;
	std::mutex m_image_mutex;

	// Pointers to the pool
	std::shared_ptr<splat_bucket_pool> m_clean_pool;
	std::shared_ptr<splat_bucket_pool> m_dirty_pool;

private:
	void new_buckets(int count, int size);

	// The main context
	std::shared_ptr<bu::rt_context> m_context;

	// Childrens' futures and active flag
	std::shared_ptr<std::atomic<bool>> m_active;
	std::vector<std::future<bool>> m_futures;
};

}