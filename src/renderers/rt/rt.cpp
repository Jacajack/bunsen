#include "rt.hpp"
#include <vector>
#include "../../log.hpp"

using namespace std::chrono_literals;
using bu::rt_renderer_job;
using bu::rt_renderer;
using bu::rt_context;

rt_context::rt_context() :
	draw_sampled_image(std::make_unique<bu::shader_program>(bu::load_shader_program("draw_sampled_image")))
{
}

rt_renderer::rt_renderer(std::shared_ptr<rt_context> context) :
	m_context(std::move(context))
{
	set_viewport_size({1024, 1024});
	LOG_DEBUG << "Created a new RT renderer instance!";
}

void rt_renderer::new_texture_storage(const glm::ivec2 &size)
{
	LOG_DEBUG << "Creating new " << size.x << "x" << size.y << " texture for rt_renderer";
	m_result_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, size.x, size.y);
	m_result_tex_size = size;
}

/**
	Allocates texture of proper size on the GPU if necessary
*/
void rt_renderer::set_viewport_size(const glm::ivec2 &viewport_size)
{
	// Calculate optimal texture size
	glm::ivec2 size = glm::exp2(glm::ceil(glm::log2(glm::vec2(viewport_size))));
	if (m_result_tex_size != size)
		new_texture_storage(size);

	m_viewport = viewport_size;
}

void rt_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	bool changed = false;

	// Detect viewport change
	if (viewport_size != m_viewport)
	{
		set_viewport_size(viewport_size);
		changed = true;
	}

	// Detect camera change
	if (camera != m_camera)
	{
		m_camera = camera;
		changed = true;
	}

	// If changed anything, reset the timer and stop the job
	if (changed)
	{
		m_last_change = std::chrono::steady_clock::now();
		if (m_job) m_job->stop();
		m_active = false;
	}

	// If 0.5 has passed from the last change, start a new job
	if (!m_active && std::chrono::steady_clock::now() - m_last_change > 0.5s)
	{
		m_job = std::make_shared<bu::rt_renderer_job>(m_context, m_camera, m_viewport);
		m_job->start();
		m_active = true;
	}

	if (m_active)
	{
		// Update image
		m_job->update();	

		// Discard old PBO contents and buffer new data
		const auto &image_data = m_job->get_image().data;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo.id());
		glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), image_data.data(), GL_STREAM_DRAW);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), nullptr, GL_STREAM_DRAW);

		// Copy texture data from the PBO
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport_size.x, viewport_size.y, GL_RGBA, GL_FLOAT, nullptr);

		glDisable(GL_DEPTH_TEST);
		glUseProgram(m_context->draw_sampled_image->id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glUniform1i(m_context->draw_sampled_image->get_uniform_location("tex"), 0);
		glUniform2i(m_context->draw_sampled_image->get_uniform_location("size"), m_viewport.x, m_viewport.y);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}
}



rt_renderer_job::rt_renderer_job(std::shared_ptr<bu::rt_context> context, const bu::camera &camera, const glm::ivec2 &viewport_size) :
	m_context(std::move(context)),
	m_ray_caster(camera),
	m_image(viewport_size)
{
}

rt_renderer_job::~rt_renderer_job()
{
	stop();
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

void rt_renderer_job::start()
{
	if (m_workers.empty())
	{
		for (int i = 0; i < 4; i++)
			m_workers.push_back(std::make_shared<bu::rt::cpu_worker>(shared_from_this()));
	
		new_buckets(64, 64 * 64);
	}

	for (auto &wp : m_workers)
		wp->start();
}

void rt_renderer_job::stop()
{
	for (auto &wp : m_workers)
		wp->stop();
}

void rt_renderer_job::new_buckets(int count, int size)
{
	LOG_INFO << "Creating " << count << " new splat buckets (size = " << size << ")";
	std::lock_guard lock{m_clean_buckets_mutex};
	
	while (count--)
		m_clean_buckets.emplace_back(std::make_unique<rt::splat_bucket>(size));
}	