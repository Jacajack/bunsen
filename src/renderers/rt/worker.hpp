#pragma once
#include <memory>
#include <atomic>
#include <future>
#include "sampled_image.hpp"

namespace bu {
	struct rt_renderer;
	struct rt_renderer_job;
	struct rt_context;
}

namespace bu::rt {

/**
	The workers are interfaces between the renderer's instance and the actual
	code doing the path tracing.

	Workers are accessed by shared pointers - this is because worker may
	not be destructed as long as its job is running.
*/
struct worker
{
	std::weak_ptr<bu::rt_renderer_job> parent_job;
	std::shared_ptr<bu::rt_context> context;
	std::unique_ptr<bu::rt::splat_bucket> bucket;
	std::atomic<bool> active = 0;

	worker(std::shared_ptr<bu::rt_renderer_job> p);
	virtual void start() = 0;
	virtual void stop();
	virtual ~worker();

	bool acquire_bucket();
	void submit_bucket();
};

struct cpu_worker : public worker
{
	std::future<bool> fut;

	using worker::worker;
	void start() override;
	void stop() override;
	~cpu_worker() override;

	static bool job(cpu_worker *worker);
};

}