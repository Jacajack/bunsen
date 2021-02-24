#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <future>
#include "../../camera.hpp"
#include "sampled_image.hpp"

namespace bu {

struct rt_context;

/**
	Path tracing rendering job. The parameters cannot be changed once the job
	has been created.
*/
class rt_renderer_job : public std::enable_shared_from_this<rt_renderer_job>
{
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

	// Childrens' futures and active flag
	std::shared_ptr<std::atomic<bool>> m_active;
	std::vector<std::future<bool>> m_futures;
};

}