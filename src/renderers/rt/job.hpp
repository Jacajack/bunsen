#pragma once
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <future>
#include <thread>
#include <optional>
#include "../../camera.hpp"
#include "sampled_image.hpp"

namespace bu {
namespace rt {
struct bvh_tree;
struct material;
}

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
	
	const rt::sampled_image &get_image() const;
	const bu::rt_context &get_context() const;
	bu::camera_ray_caster get_ray_caster() const;
	std::shared_ptr<splat_bucket_pool> get_clean_pool() const;
	std::shared_ptr<splat_bucket_pool> get_dirty_pool() const;
	std::shared_ptr<const bu::rt::bvh_tree> get_bvh() const;
	std::shared_ptr<const std::vector<bu::rt::material>> get_materials() const;

	// Run control
	void start(
		std::shared_ptr<const bu::rt::bvh_tree> bvh,
		std::shared_ptr<const std::vector<bu::rt::material>> materials,
		bu::camera &camera,
		const glm::ivec2 &viewport_size);
	void stop();

	// The sampled image
	std::shared_ptr<rt::sampled_image> m_image;
	std::mutex m_image_mutex;

private:
	void new_buckets(int count, int size);

	// The main context
	std::shared_ptr<bu::rt_context> m_context;

	// Childrens' futures and active flag
	std::shared_ptr<std::atomic<bool>> m_active;
	std::vector<std::future<bool>> m_futures;

	// Pointers to the bucket pools
	std::shared_ptr<splat_bucket_pool> m_clean_pool;
	std::shared_ptr<splat_bucket_pool> m_dirty_pool;

	// The BVH and materials
	std::shared_ptr<const bu::rt::bvh_tree> m_bvh;
	std::shared_ptr<const std::vector<bu::rt::material>> m_materials;

	// View settings
	std::optional<bu::camera_ray_caster> m_ray_caster;
};

}