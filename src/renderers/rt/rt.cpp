#include "rt.hpp"
#include <vector>
#include <chrono>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#include "job.hpp"
#include "scene_cache.hpp"
#include "bvh_builder.hpp"
#include "bvh.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "../../log.hpp"

using namespace std::chrono_literals;
using bu::rt_renderer;
using bu::rt_context;

rt_context::rt_context(bu::event_bus &bus, std::shared_ptr<bu::basic_preview_context> preview_ctx) :
	m_events(bus.make_connection()),
	m_sampled_image_program(std::make_unique<bu::shader_program>(bu::load_shader_program("draw_sampled_image"))),
	m_aabb_program(std::make_unique<bu::shader_program>(bu::load_shader_program("aabb"))),
	m_scene_cache(std::make_unique<bu::rt::scene_cache>())
{
	if (!preview_ctx) preview_ctx = std::make_shared<bu::basic_preview_context>();
	m_preview_context = std::move(preview_ctx);

	// Setup AABB VAO
	glBindVertexArray(m_aabb_vao.id());
	glVertexArrayAttribFormat( // Position (0)
		m_aabb_vao.id(), 
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0
	);

	// Enable position attribute
	glEnableVertexArrayAttrib(m_aabb_vao.id(), 0);

	// All data is read from buffer bound to binding 0
	glVertexArrayAttribBinding(m_aabb_vao.id(), 0, 0);
}

static std::unique_ptr<bu::rt::bvh_draft> build_bvh_draft(
	const bu::async_stop_flag *flag,
	rt_context *ctx)
{
	auto cache_ptr = ctx->get_scene_cache();
	auto draft_ptr = std::make_unique<bu::rt::bvh_draft>();
	draft_ptr->build(*cache_ptr, *flag);
	return draft_ptr;
}

static std::unique_ptr<bu::rt::scene> build_rt_scene(
	const bu::async_stop_flag *flag,
	rt_context *ctx,
	std::shared_ptr<bu::rt::bvh_draft> draft_ptr)
{
	auto cache_ptr = ctx->get_scene_cache();
	auto materials = std::make_shared<std::vector<bu::rt::material>>(cache_ptr->get_materials());
	
	// Build BVH
	auto bvh = std::make_shared<bu::rt::bvh_tree>(draft_ptr->get_height(), draft_ptr->get_triangle_count());
	bvh->populate(*draft_ptr);

	auto scene = std::make_unique<bu::rt::scene>();
	scene->bvh = bvh;
	scene->materials = materials;
	return scene;
}

void rt_context::update_from_scene(const bu::scene &scene, bool allow_build)
{
	const auto policy = std::launch::async;

	if (scene.root_node->is_visibly_modified() && allow_build)
	{
		auto [update_scene, update_bvh] = m_scene_cache->update_from_scene(scene);
		if (update_bvh)
		{
			LOG_INFO << "BVH cache modified - initiating draft build!";

			// Kill running tasks and start building a new BVH
			m_bvh_draft_build_task.reset();
			m_scene_build_task.reset();
			m_bvh_draft_build_task = bu::make_async_task(bu::global_task_cleaner, policy, build_bvh_draft, this);
		}
		else if (m_scene)
		{
			LOG_INFO << "Preserving BVH through scene update!";
			
			auto new_scene = std::make_shared<rt::scene>();
			new_scene->bvh = m_scene->bvh;
			new_scene->materials = std::make_shared<std::vector<rt::material>>(m_scene_cache->get_materials());
			m_scene = std::move(new_scene);
		}
	}

	// When BVH draft is complete, update BVH preview and start scene build
	if (m_bvh_draft_build_task.has_value() && m_bvh_draft_build_task->is_ready())
	{
		LOG_INFO << "BVH draft complete!";

		std::shared_ptr<bu::rt::bvh_draft> draft{std::move(m_bvh_draft_build_task->get())};
		auto aabbs = draft->get_tree_aabbs();
		glNamedBufferData(m_aabb_buffer.id(), 0, nullptr, GL_STATIC_DRAW);
		glNamedBufferData(m_aabb_buffer.id(), bu::vector_size(aabbs), aabbs.data(), GL_STATIC_DRAW);
		m_aabb_count = aabbs.size();

		m_scene_build_task = bu::make_async_task(bu::global_task_cleaner, policy, build_rt_scene, this, draft);
		m_bvh_draft_build_task.reset();
	}

	// Update scene
	if (m_scene_build_task.has_value() && m_scene_build_task->is_ready())
	{
		this->m_scene = std::move(m_scene_build_task->get());
		m_scene_build_task.reset();
		LOG_INFO << "Final BVH build finished: " << this->m_scene->bvh->triangle_count << " triangles and " << this->m_scene->bvh->node_count << " nodes";
	}	
}

rt_renderer::rt_renderer(std::shared_ptr<rt_context> context) :
	m_context(std::move(context)),
	m_preview_renderer(std::make_unique<bu::basic_preview_renderer>(m_context->get_basic_preview_context())),
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

	// Detect BVH update
	if (m_last_scene != m_context->get_scene().get())
	{
		m_last_scene = m_context->get_scene().get();
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
	if (!m_active && m_context->get_scene() && std::chrono::steady_clock::now() - m_last_change > 0.5s)
	{
		if (m_job) m_job->stop();
		m_job->start(m_context->get_scene(), m_camera, m_viewport, 512);
		m_active = true;
	}

	// Draw preview
	if (m_preview_active)
	{
		ZoneScopedN("RT preview");
		m_preview_renderer->draw(scene, camera, viewport_size);
	}

	// BVH preview
	if (m_preview_active && m_context->get_aabb_count())
	{
		auto mat_view = camera.get_view_matrix();
		auto mat_proj = camera.get_projection_matrix();
		glm::vec3 line_color{1, 1, 0};

		auto &aabb_program = m_context->get_aabb_program();
		glUseProgram(aabb_program.id());
		glUniformMatrix4fv(aabb_program.get_uniform_location("mat_view"), 1, GL_FALSE, &mat_view[0][0]);
		glUniformMatrix4fv(aabb_program.get_uniform_location("mat_proj"), 1, GL_FALSE, &mat_proj[0][0]);
		glUniform3fv(aabb_program.get_uniform_location("color"), 1, &line_color[0]);

		glBindVertexArray(m_context->get_aabb_vao().id());
		glBindVertexBuffer(0, m_context->get_aabb_buffer().id(), 0, 3 * sizeof(float));
		glDrawArrays(GL_LINES, 0, 2 * m_context->get_aabb_count());
		glBindVertexBuffer(0, 0, 0, 0);
	}

	// Draw the sampled image if the job is active
	if (m_active)
	{
		// Discard old PBO contents and buffer new data
		{
			ZoneScopedN("PBO upload")

			auto ctx = m_job->get_job_context();
			ctx->inhibit_splat = true;
			{
				std::lock_guard lock{ctx->image_mutex};
				ZoneScopedN("PBO upload image lock");

				const auto &image_data = m_job->get_image().data;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo.id());
				glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), nullptr, GL_STREAM_DRAW);
				glBufferData(GL_PIXEL_UNPACK_BUFFER, bu::vector_size(image_data), image_data.data(), GL_STREAM_DRAW);
			}
			ctx->inhibit_splat = false;
		}

		// Copy texture data from the PBO
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport_size.x, viewport_size.y, GL_RGBA, GL_FLOAT, nullptr);

		ZoneScopedN("Draw");
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glUseProgram(m_context->get_sampled_image_program().id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_result_tex->id());
		glUniform1i(m_context->get_sampled_image_program().get_uniform_location("tex"), 0);
		glUniform2i(m_context->get_sampled_image_program().get_uniform_location("size"), m_viewport.x, m_viewport.y);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		
		// Unbind PBO
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	FrameMarkEnd(tracy_frame);
}
