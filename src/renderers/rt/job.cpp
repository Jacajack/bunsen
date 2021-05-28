#include "job.hpp"
#include <random>
#include <thread>
#include <tracy/Tracy.hpp>
#include "../../log.hpp"
#include "ray.hpp"
#include "bvh.hpp"
#include "rt.hpp"
#include <glm/gtx/string_cast.hpp>

using namespace std::chrono_literals;
using bu::splat_bucket_pool;
using bu::rt_renderer_job;

static bool child_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep);
static bool splatter_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep);

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

/**
	Sets up a new activity flag for the new children
	and spawns them. Accepts paremeters for the new render
*/
void rt_renderer_job::start(const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	if (m_active && *m_active) stop();

	// Remove completed futures
	m_futures.erase(std::remove_if(m_futures.begin(), m_futures.end(), [](auto &f){
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}), m_futures.end());

	m_active = std::make_shared<std::atomic<bool>>(true);
	m_ray_caster = std::make_shared<bu::camera_ray_caster>(camera);
	m_image = std::make_shared<bu::rt::sampled_image>(viewport_size);
	m_clean_pool = std::make_shared<splat_bucket_pool>();
	m_dirty_pool = std::make_shared<splat_bucket_pool>();
	new_buckets(64, 64 * 64);

	for (int i = 0; i < 4; i++)
		m_futures.emplace_back(std::async(std::launch::async, child_job, this, m_active));

	m_futures.emplace_back(std::async(std::launch::async, splatter_job, this, m_active));
}


/**
	Stops the rendering and lets the threads die on their own
*/
void rt_renderer_job::stop()
{
	if (m_active) *m_active = false;
}

void rt_renderer_job::new_buckets(int count, int size)
{
	LOG_INFO << "Creating " << count << " new splat buckets (size = " << size << ")";
	
	while (count--)
		m_clean_pool->submit(std::make_unique<rt::splat_bucket>(size));
}

static bool child_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep)
{
	auto &job = *jobp;
	auto &active = *activep;
	auto &ctx = job.get_context();
	auto ray_caster = *job.m_ray_caster;
	auto bvh = ctx.bvh;
	std::shared_ptr<bu::splat_bucket_pool> clean_pool = job.m_clean_pool;
	std::shared_ptr<bu::splat_bucket_pool> dirty_pool = job.m_dirty_pool;

	LOG_INFO << "RT CPU thread starting!";
	LOG_INFO << "Ray caster \n" << glm::to_string(ray_caster.matrix);

	std::mt19937 rng(124725 + std::random_device{}());
	std::uniform_int_distribution<int> idist(0, 500);
	std::uniform_real_distribution<float> fdist(0, 1);

	while (active)
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
			auto pos = glm::vec2{fdist(rng), fdist(rng)} * glm::vec2{job.get_image().size};
			auto color = glm::vec3{fdist(rng), fdist(rng), fdist(rng)};

			for (auto i = 0u; i < bucket->size; i++)
			{
				auto &splat = bucket->data[i];
				splat.pos = pos + glm::vec2{i % 64, i / 64};
				
				splat.samples = 1;

				auto ndc = (splat.pos / glm::vec2{job.get_image().size}) * 2.f - 1.f;
				auto dir = ray_caster.get_direction(ndc);
				bu::rt::ray r;

				r.direction = glm::normalize(dir);
				r.origin = ray_caster.origin;

				// LOG_DEBUG << "ray from " << r.origin.x << " " << r.origin.y << " " << r.origin.z;
				// LOG_DEBUG << "ray dir " << r.direction.x << " " << r.direction.y << " " << r.direction.z;

				bu::rt::ray_hit hit;
				bool ok = bvh->test_ray(r, hit);
				if (ok)
				{
					const auto &tri = *hit.triangle;
					glm::vec3 N = glm::normalize((1 - hit.u - hit.v) * tri.normals[0] + hit.u * tri.normals[1] + hit.v * tri.normals[2]);
					splat.color = N;
				}
				else
				{
					splat.color = glm::vec3{};
				}
			}

			dirty_pool->submit(std::move(bucket));
		}

		std::this_thread::sleep_for(0.01s);
	}

	LOG_INFO << "RT CPU thread terminating!";
	return true;
}

static bool splatter_job(rt_renderer_job *jobp, std::shared_ptr<std::atomic<bool>> activep)
{
	auto &job = *jobp;
	auto &active = *activep;
	std::shared_ptr<bu::rt::sampled_image> image = job.m_image;
	std::shared_ptr<bu::splat_bucket_pool> clean_pool = job.m_clean_pool;
	std::shared_ptr<bu::splat_bucket_pool> dirty_pool = job.m_dirty_pool;

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
