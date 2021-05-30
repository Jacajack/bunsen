#include "job.hpp"
#include <random>
#include <thread>
#include <algorithm>
#include <tracy/Tracy.hpp>
#include "../../log.hpp"
#include "ray.hpp"
#include "bvh.hpp"
#include "rt.hpp"
#include "kernel.hpp"
#include <glm/gtx/string_cast.hpp>

using namespace std::chrono_literals;
using bu::splat_bucket_pool;
using bu::rt_renderer_job;

static bool child_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep, int job_id);
static bool splatter_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep, int job_id);

void splat_bucket_pool::submit(std::unique_ptr<rt::splat_bucket> bucket)
{
	std::lock_guard lock{mut};
	buckets.emplace_back(std::move(bucket));
	cv.notify_one();
}

std::unique_ptr<bu::rt::splat_bucket> splat_bucket_pool::acquire()
{
	std::lock_guard lock{mut};
	if (buckets.empty())
		return {};
	else
	{
		auto p = std::move(buckets.back());
		buckets.pop_back();
		return p;
	}
}

rt_renderer_job::rt_renderer_job(std::shared_ptr<bu::rt_context> context) :
	m_context(std::move(context))
{
	LOG_INFO << "New RT job!";
}

/**
	This blocks until all threads die
*/
rt_renderer_job::~rt_renderer_job()
{
	if (m_active) *m_active = false;
	LOG_INFO << "RT job terminated!";
}

const bu::rt::sampled_image &rt_renderer_job::get_image() const
{
	return *m_image;
}

const bu::rt_context &rt_renderer_job::get_context() const
{
	return *m_context;
}

bu::camera_ray_caster rt_renderer_job::get_ray_caster() const
{
	return *m_ray_caster;
}

std::shared_ptr<splat_bucket_pool> rt_renderer_job::get_clean_pool() const
{
	return m_clean_pool;
}

std::shared_ptr<splat_bucket_pool> rt_renderer_job::get_dirty_pool() const
{
	return m_dirty_pool;
}

std::shared_ptr<const bu::rt::bvh_tree> rt_renderer_job::get_bvh() const
{
	return m_bvh;
}

/**
	Sets up a new activity flag for the new children
	and spawns them. Accepts paremeters for the new render
*/
void rt_renderer_job::start(std::shared_ptr<const bu::rt::bvh_tree> bvh, const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	if (m_active && *m_active) stop();

	if (!bvh)
	{
		LOG_ERROR << "Cannot start RT thread without a BVH!";
		throw std::runtime_error{"rt_renderer_job started without a valid BVH"};
	}

	// Remove completed futures
	m_futures.erase(std::remove_if(m_futures.begin(), m_futures.end(), [](auto &f){
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}), m_futures.end());

	LOG_INFO << "Starting new RT jobs";
	m_active = std::make_shared<std::atomic<bool>>(true);
	m_ray_caster = bu::camera_ray_caster{camera};
	m_image = std::make_shared<bu::rt::sampled_image>(viewport_size);
	m_clean_pool = std::make_shared<splat_bucket_pool>();
	m_dirty_pool = std::make_shared<splat_bucket_pool>();
	m_bvh = bvh;
	new_buckets(64, 64 * 64);

	int job_count = 4;

	for (int i = 0; i < job_count; i++)
		m_futures.emplace_back(std::async(std::launch::async, child_job, this, m_active, i));

	m_futures.emplace_back(std::async(std::launch::async, splatter_job, this, m_active, job_count));
}


/**
	Stops the rendering and lets the threads die on their own
*/
void rt_renderer_job::stop()
{
	if (m_active)
	{
		if (*m_active)
			LOG_INFO << "Requesting RT jobs to stop";
		 *m_active = false;
	}
	m_bvh.reset();
}

void rt_renderer_job::new_buckets(int count, int size)
{
	LOG_INFO << "Creating " << count << " new splat buckets (size = " << size << ")";
	
	while (count--)
		m_clean_pool->submit(std::make_unique<rt::splat_bucket>(size));
}

static bool child_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep, int job_id)
{
	auto &job = *jobp;
	auto &active = *activep;
	auto &image = job.get_image();
	auto ray_caster = job.get_ray_caster();
	auto bvh = job.get_bvh();
	auto clean_pool = job.get_clean_pool();
	auto dirty_pool = job.get_dirty_pool();
	std::mt19937 rng(std::random_device{}() + job_id);
	std::uniform_real_distribution<float> dist(0, 1);

	for (int bucket_id = 0; active; bucket_id++)
	{	
		std::unique_ptr<bu::rt::splat_bucket> bucket;

		// This should never stall
		while (active && !(bucket = clean_pool->acquire()))
		{
			ZoneScopedN("Bucket wait");
			LOG_WARNING << "RT thread could not acquire bucket - stalling!";
			std::this_thread::sleep_for(0.1s);
		}

		if (!active) break;
		
		{
			ZoneScopedN("Bucket generation");

			//! \todo Variable tile dimensions!
			int tile_size = 64;
			int job_count = 4;
			auto start_pos = glm::ivec2{0, (job_id * tile_size) % image.size.y};

			for (auto i = 0u; i < bucket->size && active; i++)
			{
				auto &splat = bucket->data[i];
				glm::ivec2 p = start_pos + glm::ivec2{i % tile_size, i / tile_size} + glm::ivec2{bucket_id * tile_size, 0};
				p.y += (p.x / image.size.x) * job_count * tile_size;
				p.x %= image.size.x;
				p.y %= image.size.y;
				splat.pos = glm::vec2{p} + glm::vec2{0.1, 0.1} + 0.8f * glm::vec2{dist(rng), dist(rng)};
				auto ndc = (splat.pos / glm::vec2{job.get_image().size}) * 2.f - 1.f;
				
				bu::rt::ray r;
				r.direction = ray_caster.get_direction(ndc);
				r.origin = ray_caster.origin;
				splat.color = bu::rt::trace_ray(*bvh, rng, r, 24);
				splat.samples = 1;
			}

			dirty_pool->submit(std::move(bucket));
		}
	}

	return true;
}

static bool splatter_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep, int job_id)
{
	auto &job = *jobp;
	auto &active = *activep;
	std::shared_ptr<bu::rt::sampled_image> image = job.m_image;
	auto clean_pool = job.get_clean_pool();
	auto dirty_pool = job.get_dirty_pool();

	while (active)
	{
		std::unique_ptr<bu::rt::splat_bucket> bucket;

		// If bucket can be acquired do it
		if (!(bucket = dirty_pool->acquire()))
		{
			// Otherwise wait until notified
			std::unique_lock<std::mutex> lk{dirty_pool->mut};
			dirty_pool->cv.wait_for(lk, 0.1s);

			if (!active) return true;

			// Wakeup can be spurious - check size
			if (dirty_pool->buckets.size())
			{
				bucket = std::move(dirty_pool->buckets.back());
				dirty_pool->buckets.pop_back();
			}
			else
				continue;
		}

		// Splat the bucket on the image
		{
			std::lock_guard lock{job.m_image_mutex};
			image->splat(*bucket);
		}

		// Return the bucket
		clean_pool->submit(std::move(bucket));
	}

	return true;
}
