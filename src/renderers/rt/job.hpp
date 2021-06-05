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
struct scene;
}

class rt_context;

struct splat_bucket_pool
{
	std::mutex mut;
	std::condition_variable cv;
	std::vector<std::unique_ptr<rt::splat_bucket>> buckets;

	void submit(std::unique_ptr<rt::splat_bucket> bucket);
	std::unique_ptr<rt::splat_bucket> acquire();
};

/**
	\brief Context provided to each ray-tracing thread
*/
struct rt_job_context
{
	rt_job_context(
		std::shared_ptr<const rt::scene> scene,
		const bu::camera &camera,
		const glm::ivec2 &viewport_size,
		int bucket_count,
		int thread_count,
		int tile_size);

	std::atomic<bool> active;
	
	std::shared_ptr<const bu::rt::scene> scene;
	bu::camera_ray_caster ray_caster;

	splat_bucket_pool clean_pool;
	splat_bucket_pool dirty_pool;

	rt::sampled_image image;
	std::mutex image_mutex;
	std::atomic<bool> inhibit_splat;

	int bucket_count;
	int thread_count;
	int tile_size;
};

/**
	\brief Path tracing rendering job - spawns multiple threads sharing one rt_job_context
*/
class rt_renderer_job : public std::enable_shared_from_this<rt_renderer_job>
{
public:
	rt_renderer_job(std::shared_ptr<bu::rt_context> context);
	~rt_renderer_job();
	
	auto &get_image() const {return m_job_context->image;}
	auto &get_context() const {return *m_context;}
	auto get_job_context() const {return m_job_context;}

	// Run control
	void start(
		std::shared_ptr<const bu::rt::scene> scene,
		bu::camera &camera,
		const glm::ivec2 &viewport_size,
		int bucket_count = 64,
		int thread_count = 4,
		int tile_size = 64);
	void stop();

private:
	// The main context
	std::shared_ptr<bu::rt_context> m_context;

	// Childrens' futures and active flag
	std::shared_ptr<rt_job_context> m_job_context;
	std::vector<std::future<bool>> m_futures;
};

}