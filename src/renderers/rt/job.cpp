#include "job.hpp"
#include <random>
#include <thread>
#include <tracy/Tracy.hpp>
#include "../../log.hpp"

using namespace std::chrono_literals;
using bu::rt_renderer_job;

static bool child_job(std::shared_ptr<rt_renderer_job> jobp, std::shared_ptr<std::atomic<bool>> activep);

rt_renderer_job::rt_renderer_job(std::shared_ptr<bu::rt_context> context, const bu::camera &camera, const glm::ivec2 &viewport_size) :
	m_context(std::move(context)),
	m_ray_caster(camera),
	m_image(viewport_size)
{
	LOG_INFO << "New RT job!";
}

/**
	We can't allow this to be called by any of the children, because that would
	result in a deadlock
*/
rt_renderer_job::~rt_renderer_job()
{
	LOG_INFO << "RT job terminated!";
}

void rt_renderer_job::submit_bucket(std::unique_ptr<rt::splat_bucket> bucket)
{
	std::lock_guard lock{m_dirty_buckets_mutex};
	m_dirty_buckets.emplace_back(std::move(bucket));
}

std::unique_ptr<bu::rt::splat_bucket> rt_renderer_job::acquire_bucket()
{
	std::lock_guard lock{m_clean_buckets_mutex};
	if (m_clean_buckets.empty())
		return {};
	else
	{
		auto p = std::move(m_clean_buckets.back());
		m_clean_buckets.pop_back();
		return p;
	}
}

const bu::rt::sampled_image &rt_renderer_job::get_image() const
{
	return m_image;
}

void rt_renderer_job::update()
{
	ZoneScoped;

	// Splat all dirty buckets
	while (m_dirty_buckets.size())
	{
		std::unique_ptr<rt::splat_bucket> bucket;
		{
			std::lock_guard lock{m_dirty_buckets_mutex};
			if (!m_dirty_buckets.size()) break;
			bucket = std::move(m_dirty_buckets.back());
			m_dirty_buckets.pop_back();
		}

		m_image.splat(*bucket);

		{
			std::lock_guard lock{m_clean_buckets_mutex};
			m_clean_buckets.push_back(std::move(bucket));
		}
	}
}

/**
	Sets up a new activity flag for the new children
	and spawns them
*/
void rt_renderer_job::start()
{
	stop();
	m_active = std::make_shared<std::atomic<bool>>(true);
	new_buckets(64, 64 * 64);
	for (int i = 0; i < 4; i++)
		m_futures.emplace_back(std::async(std::launch::async, child_job, shared_from_this(), m_active));
}

/**
	Resets activity flag and discards all the futures.
	Clearing the futures vector blocks until all of them are ready.
*/
void rt_renderer_job::stop()
{
	if (m_active) *m_active = false;
	m_futures.clear();
}

void rt_renderer_job::new_buckets(int count, int size)
{
	LOG_INFO << "Creating " << count << " new splat buckets (size = " << size << ")";
	std::lock_guard lock{m_clean_buckets_mutex};
	
	while (count--)
		m_clean_buckets.emplace_back(std::make_unique<rt::splat_bucket>(size));
}

static bool child_job(std::shared_ptr<rt_renderer_job> jobp, std::shared_ptr<std::atomic<bool>> activep)
{
	auto &job = *jobp;
	auto &active = *activep;

	LOG_INFO << "RT CPU thread starting!";

	std::mt19937 rng(124725 + std::random_device{}());
	std::uniform_int_distribution<int> idist(0, 500);
	std::uniform_real_distribution<float> fdist(0, 1);

	while (active)
	{	
		std::unique_ptr<bu::rt::splat_bucket> bucket;
		while (active && !(bucket = job.acquire_bucket()))
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
				splat.color = color;
				splat.samples = 1;
			}

			job.submit_bucket(std::move(bucket));
		}

		std::this_thread::sleep_for(0.01s);
	}

	LOG_INFO << "RT CPU thread terminating!";
	return true;
}
