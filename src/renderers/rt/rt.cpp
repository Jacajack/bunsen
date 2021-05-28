#include "rt.hpp"
#include <vector>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "job.hpp"
#include "bvh_builder.hpp"
#include "bvh.hpp"
#include "../../log.hpp"

using namespace std::chrono_literals;
using bu::rt_renderer;
using bu::rt_context;

static std::unique_ptr<bu::rt::bvh_draft> build_bvh_draft(
	const bu::async_stop_flag *flag,
	rt_context *ctx)
{
	auto cache_ptr = ctx->bvh_cache;
	return std::make_unique<bu::rt::bvh_draft>(cache_ptr->build_draft(*flag));
}

static std::unique_ptr<bu::rt::bvh_tree> build_bvh_tree(
	const bu::async_stop_flag *flag,
	rt_context *ctx)
{
	auto draft = ctx->bvh_draft;
	auto bvh = std::make_unique<bu::rt::bvh_tree>(draft->get_height(), draft->get_triangle_count());
	bvh->populate(*draft);
	return bvh;
}


rt_context::rt_context(std::shared_ptr<bu::basic_preview_context> preview_ctx) :
	draw_sampled_image(std::make_unique<bu::shader_program>(bu::load_shader_program("draw_sampled_image"))),
	draw_aabb(std::make_unique<bu::shader_program>(bu::load_shader_program("aabb"))),
	bvh_cache(std::make_unique<bu::rt::bvh_cache>())
{
	if (!preview_ctx) preview_ctx = std::make_shared<bu::basic_preview_context>();
	preview_context = std::move(preview_ctx);

	// Setup AABB VAO
	glBindVertexArray(aabb_vao.id());
	glVertexArrayAttribFormat( // Position (0)
		aabb_vao.id(), 
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0
	);

	// Enable position attribute
	glEnableVertexArrayAttrib(aabb_vao.id(), 0);

	// All data is read from buffer bound to binding 0
	glVertexArrayAttribBinding(aabb_vao.id(), 0, 0);
}

void rt_context::update_bvh(const bu::scene &scene, bool rebuild)
{
	const auto policy = std::launch::async;

	if (rebuild)
	{
		bool cache_modified = bvh_cache->update_from_scene(scene);
		if (cache_modified)
		{
			LOG_INFO << "BVH cache modified - initiating draft build!";
			if (bvh_draft_build_task.has_value())
				bu::discard_task(bu::global_task_cleaner, std::move(*bvh_draft_build_task));
			bvh_draft_build_task = bu::make_async_task(bu::global_task_cleaner, policy, build_bvh_draft, this);
		}
	}

	if (bvh_draft_build_task.has_value() && bu::is_future_ready(*bvh_draft_build_task))
	{
		bvh_draft = std::move(bvh_draft_build_task->get());

		if (bvh_build_task.has_value())
			bu::discard_task(bu::global_task_cleaner, std::move(*bvh_build_task));
		bvh_build_task = bu::make_async_task(bu::global_task_cleaner, policy, build_bvh_tree, this);
		bvh_draft_build_task.reset();
		LOG_INFO << "BVH draft complete: " << bvh_draft->get_triangle_count() << " triangles and " << bvh_draft->get_height() << " levels";
		LOG_INFO << "Initiating final BVH build";
	}

	if (bvh_build_task.has_value() && bu::is_future_ready(*bvh_build_task))
	{
		bvh = std::move(bvh_build_task->get());
		bvh_build_task.reset();
		LOG_INFO << "Final BVH build finished: " << bvh->triangle_count << " triangles and " << bvh->node_count << " nodes";
	}		
}

rt_renderer::rt_renderer(std::shared_ptr<rt_context> context) :
	m_context(std::move(context)),
	m_preview_renderer(std::make_unique<bu::basic_preview_renderer>(m_context->preview_context)),
	m_job(std::make_unique<bu::rt_renderer_job>(m_context))
{
	set_viewport_size({1024, 1024});
	LOG_DEBUG << "Created a new RT renderer instance!";
}

rt_renderer::~rt_renderer()
{
	if (m_job) m_job->stop();
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

void rt_renderer::update()
{
}

void rt_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size)
{
	const char *tracy_frame = "rt_renderer::draw()";
	FrameMarkStart(tracy_frame);
	ZoneScopedN("rt_renderer::draw()");

	bool changed = scene.layout_ed.is_transform_pending();

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
		if (m_job) m_job->stop();
		m_job->start(m_camera, m_viewport);
		m_active = true;
	}

	// Draw preview
	if (m_preview_active)
	{
		ZoneScopedN("RT preview");
		m_preview_renderer->draw(scene, camera, viewport_size);
	}

	// BVH preview
	if (m_preview_active && m_context->bvh_draft)
	{
		auto mat_view = camera.get_view_matrix();
		auto mat_proj = camera.get_projection_matrix();
		glm::vec3 line_color{1, 1, 0};

		std::vector<glm::vec3> aabb_data;
		auto aabbs = m_context->bvh_draft->get_tree_aabbs();
		for (auto &box : aabbs)
		{
			aabb_data.push_back(box.min);
			aabb_data.push_back(box.max);
		}

		auto &aabb_program = m_context->draw_aabb;
		glBindVertexArray(m_context->aabb_vao.id());
		glUseProgram(aabb_program->id());
		glUniformMatrix4fv(aabb_program->get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
		glUniformMatrix4fv(aabb_program->get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
		glUniform3fv(aabb_program->get_uniform_location("color"), 1, &line_color[0]);

		glNamedBufferData(aabb_buffer.id(), bu::vector_size(aabb_data), nullptr, GL_STATIC_DRAW);
		glNamedBufferData(aabb_buffer.id(), bu::vector_size(aabb_data), aabb_data.data(), GL_STATIC_DRAW);

		glBindVertexBuffer(0, aabb_buffer.id(), 0, 3 * sizeof(float));
		glDrawArrays(GL_LINES, 0, aabb_data.size());
	}

	// Draw the sampled image if the job is active
	if (m_active)
	{
		// Discard old PBO contents and buffer new data
		{
		ZoneScopedN("PBO upload")
		std::lock_guard lock{m_job->m_image_mutex};
		const auto &image_data = m_job->m_image->data;
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo.id());
		glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), nullptr, GL_STREAM_DRAW);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), image_data.data(), GL_STREAM_DRAW);
		}

		// Copy texture data from the PBO
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport_size.x, viewport_size.y, GL_RGBA, GL_FLOAT, nullptr);

		ZoneScopedN("Draw");
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glUseProgram(m_context->draw_sampled_image->id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glUniform1i(m_context->draw_sampled_image->get_uniform_location("tex"), 0);
		glUniform2i(m_context->draw_sampled_image->get_uniform_location("size"), m_viewport.x, m_viewport.y);
		// glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}

	FrameMarkEnd(tracy_frame);
}
