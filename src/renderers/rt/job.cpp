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
#include "scene.hpp"
#include <glm/gtx/string_cast.hpp>

using namespace std::chrono_literals;
using bu::splat_bucket_pool;
using bu::rt_renderer_job;
using bu::rt_job_context;

static bool child_job(std::shared_ptr<rt_job_context> ctx, int job_id);
static bool splatter_job(std::shared_ptr<rt_job_context> ctx, int job_id);

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

rt_job_context::rt_job_context(
	std::shared_ptr<const rt::scene> scene,
	const bu::camera &camera,
	const glm::ivec2 &viewport_size,
	int bucket_count,
	int thread_count,
	int tile_size) :
	active(true),
	scene(std::move(scene)),
	ray_caster(camera),
	image(viewport_size),
	inhibit_splat(false),
	bucket_count(bucket_count),
	thread_count(thread_count),
	tile_size(tile_size)
{
	int bucket_size = tile_size * tile_size;
	LOG_INFO << "Creating " << bucket_count << " new splat buckets (size = " << bucket_size << ")";
	for (int i = 0; i < bucket_count; i++)
		clean_pool.submit(std::make_unique<rt::splat_bucket>(bucket_size));
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
	if (m_job_context && m_job_context->active)
		m_job_context->active = false;
	LOG_INFO << "RT job terminated!";
}

/**
	Sets up a new activity flag for the new children
	and spawns them. Accepts paremeters for the new render
*/
void rt_renderer_job::start(
	std::shared_ptr<const bu::rt::scene> scene,
	bu::camera &camera,
	const glm::ivec2 &viewport_size,
	int bucket_count,
	int thread_count,
	int tile_size)
{
	if (m_job_context && m_job_context->active)
		stop();

	if (!scene)
	{
		LOG_ERROR << "Cannot start RT thread without a scene!";
		throw std::runtime_error{"rt_renderer_job started without a valid scene"};
	}

	// Remove completed futures
	m_futures.erase(std::remove_if(m_futures.begin(), m_futures.end(), [](auto &f){
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}), m_futures.end());

	m_job_context = std::make_shared<rt_job_context>(
		scene,
		camera,
		viewport_size,
		bucket_count,
		thread_count,
		tile_size);

	LOG_INFO << "Starting new RT jobs";
	for (int i = 0; i < thread_count; i++)
		m_futures.emplace_back(std::async(std::launch::async, child_job, m_job_context, i));

	m_futures.emplace_back(std::async(std::launch::async, splatter_job, m_job_context, thread_count));
}


/**
	Stops the rendering and lets the threads die on their own
*/
void rt_renderer_job::stop()
{
	if (m_job_context)
	{
		if (m_job_context->active)
			LOG_INFO << "Requesting RT jobs to stop";
		 m_job_context->active = false;
	}
	m_job_context.reset();
}


static bool child_job(std::shared_ptr<rt_job_context> ctx, int job_id)
{
	std::mt19937 rng(std::random_device{}() + job_id);
	std::uniform_real_distribution<float> dist(0, 1);

	for (int bucket_id = 0; ctx->active; bucket_id++)
	{	
		std::unique_ptr<bu::rt::splat_bucket> bucket;

		// This should never stall
		while (ctx->active && !(bucket = ctx->clean_pool.acquire()))
		{
			ZoneScopedN("Bucket wait");
			LOG_WARNING << "RT thread could not acquire bucket - stalling!";
			std::this_thread::sleep_for(0.1s);
		}

		if (!ctx->active) break;
		
		{
			ZoneScopedN("Bucket generation");

			auto start_pos = glm::ivec2{0, (job_id * ctx->tile_size) % ctx->image.size.y};

			for (auto i = 0u; i < bucket->size && ctx->active; i++)
			{
				auto &splat = bucket->data[i];
				glm::ivec2 p = start_pos + glm::ivec2{i % ctx->tile_size, i / ctx->tile_size} + glm::ivec2{bucket_id * ctx->tile_size, 0};
				p.y += (p.x / ctx->image.size.x) * ctx->thread_count * ctx->tile_size;
				p.x %= ctx->image.size.x;
				p.y %= ctx->image.size.y;
				splat.pos = glm::vec2{p} + glm::vec2{dist(rng), dist(rng)};
				auto ndc = (splat.pos / glm::vec2{ctx->image.size}) * 2.f - 1.f;
				
				bu::rt::ray r;
				r.direction = ctx->ray_caster.get_direction(ndc);
				r.origin = ctx->ray_caster.origin;
				splat.color = bu::rt::trace_ray(*ctx->scene->bvh, *ctx->scene->materials, rng, r, 24);
				splat.samples = 1;
			}

			ctx->dirty_pool.submit(std::move(bucket));
		}
	}

	return true;
}

static bool splatter_job(std::shared_ptr<rt_job_context> ctx, int job_id)
{
	while (ctx->active)
	{
		std::unique_ptr<bu::rt::splat_bucket> bucket;

		// If bucket can be acquired do it
		if (!(bucket = ctx->dirty_pool.acquire()))
		{
			// Otherwise wait until notified
			std::unique_lock<std::mutex> lk{ctx->dirty_pool.mut};
			ctx->dirty_pool.cv.wait_for(lk, 0.1s);

			if (!ctx->active) return true;

			// Wakeup can be spurious - check size
			if (ctx->dirty_pool.buckets.size())
			{
				bucket = std::move(ctx->dirty_pool.buckets.back());
				ctx->dirty_pool.buckets.pop_back();
			}
			else
				continue;
		}

		// Wait if splatting is inhibited
		while (ctx->inhibit_splat && ctx->active)
			std::this_thread::sleep_for(1ms);

		// Splat the bucket on the image
		{
			std::lock_guard lock{ctx->image_mutex};
			ZoneScopedN("Splat image lock");
			ctx->image.splat(*bucket);
		}

		// Return the bucket
		ctx->clean_pool.submit(std::move(bucket));
	}

	return true;
}
