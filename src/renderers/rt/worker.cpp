#include "worker.hpp"
#include <future>
#include <random>
#include "rt.hpp"
#include "../../log.hpp"

using namespace std::chrono_literals;
using bu::rt::cpu_worker;
using bu::rt::worker;

worker::worker(std::shared_ptr<bu::rt_renderer_job> p) :
	parent_job(p),
	context(p->m_context)
{
}

void worker::stop()
{
	active = false;
}

worker::~worker()
{
	stop();
}

bool worker::acquire_bucket()
{
	if (auto p = parent_job.lock())
	{
		if (auto b = p->acquire_bucket())
		{
			bucket = std::move(b);
			return true;
		}
	}

	return false;
}

void worker::submit_bucket()
{
	if (auto p = parent_job.lock())
		p->submit_bucket(std::move(bucket));
	else
	{
		LOG_WARNING << "Discarding bucket!";
		bucket.reset();
	}
}

void cpu_worker::start()
{
	active = true;
	fut = std::async(std::launch::async, job, this);
}

void cpu_worker::stop()
{
	active = false;
	fut.wait();
}

cpu_worker::~cpu_worker()
{
	stop();
}

bool cpu_worker::job(cpu_worker *worker)
{
	auto &job = *worker->parent_job.lock();

	LOG_INFO << "RT CPU worker thread starting!";

	std::mt19937 rng(124725 + std::random_device{}());
	std::uniform_int_distribution<int> idist(0, 500);
	std::uniform_real_distribution<float> fdist(0, 1);

	while (worker->active)
	{	
		while (worker->active && !worker->acquire_bucket())
		{
			LOG_WARNING << "RT worker thread could not acquire bucket - stalling!";
			std::this_thread::sleep_for(0.1s);
		}

		if (!worker->active) break;

		auto pos = glm::vec2{fdist(rng), fdist(rng)} * glm::vec2{job.get_image().size};
		auto color = glm::vec3{fdist(rng), fdist(rng), fdist(rng)};

		for (auto i = 0u; i < worker->bucket->size; i++)
		{
			auto &splat = worker->bucket->data[i];
			splat.pos = pos + glm::vec2{i % 64, i / 64};
			splat.color = color;
			splat.samples = 1;

		}

		std::this_thread::sleep_for(0.01s);

		worker->submit_bucket();
		// LOG_INFO << "submitting bucket!";
	}

	LOG_INFO << "RT CPU worker thread terminating!";
	return true;
}