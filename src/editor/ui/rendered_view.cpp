#include "rendered_view.hpp"
#include <imgui.h>
#include "../../log.hpp"
#include "../../utils.hpp"

#include "../../renderers/albedo/albedo.hpp"
#include "../../renderers/preview/preview.hpp"
#include "../../renderers/basic_preview/basic_preview.hpp"
#include "../../renderers/rt/rt.hpp"

using bu::ui::rendered_view;
using bu::ui::rendered_view_window;

rendered_view::rendered_view(int samples)
{
	if (samples < 0) m_samples = bu::bunsen::get().config.general.msaa;
	else m_samples = samples;
	m_downsample_program = std::make_unique<shader_program>(bu::load_shader_program("downsample_msaa"));
}

/**
	\brief Creates new texture of given size and with given number of samples

	MS and "normal" textures are bound to two separate FBOs.
*/
void rendered_view::new_texture_storage(int w, int h, int samples)
{
	LOG_DEBUG << "Creating new " << w << "x" << h << " (" << samples << " samples" << ") texture storage for rendered_view";
	m_fbo_tex_size = glm::ivec2(w, h);
	m_samples = samples;

	// Sadly, we can't use glTextureStorage2D() because it's from GL 4.5
	// Also, glTexStorage2DMultisample() is GL 4.3 and higher

	const GLenum color_format = GL_RGBA8;
	const GLenum depth_format = GL_DEPTH32F_STENCIL8;

	if (samples)
	{
		// MS color texture
		m_color_ms_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D_MULTISAMPLE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_color_ms_tex->id());
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, color_format, w, h, GL_FALSE);

		// MS depth texture
		m_depth_stencil_ms_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D_MULTISAMPLE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_depth_stencil_ms_tex->id());
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, depth_format, w, h, GL_FALSE);

		// Bind to FBO
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_ms.id());
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_color_ms_tex->id(), 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_depth_stencil_ms_tex->id(), 0);
	}
	else
	{
		m_color_ms_tex.reset();
		m_depth_stencil_ms_tex.reset();
	}

	// Color texture
	m_color_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_color_tex->id());
	glTexStorage2D(GL_TEXTURE_2D, 1, color_format, w, h);

	// Depth
	m_depth_stencil_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_depth_stencil_tex->id());
	glTexStorage2D(GL_TEXTURE_2D, 1, depth_format, w, h);

	// Bind to FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo.id());
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_tex->id(), 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth_stencil_tex->id(), 0);
}

/**
	\brief Calls new_texture_storage() if the provided content size is too big
		or too large

	If samples is -1, the MSAA sampling remains unchanged
*/
void rendered_view::set_content_size(const glm::vec2 &content_size, int samples)
{
	if (samples < 0) samples = m_samples;

	// Calculate optimal size
	int w = std::exp2(std::ceil(std::log2(content_size.x)));
	int h = std::exp2(std::ceil(std::log2(content_size.y)));
	glm::ivec2 size{w, h};

	if (samples != m_samples || m_fbo_tex_size != size)
		new_texture_storage(size.x, size.y, samples);
}

void rendered_view::draw(const bu::scene &scene, const bu::camera &camera)
{
	ImVec2 content_size = ImGui::GetContentRegionAvail();
	ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

	// Update texture size
	set_content_size(bu::to_vec2(content_size));

	before();

	// Draw to MSAA or normal FBO
	glBindFramebuffer(GL_FRAMEBUFFER, m_samples ? m_fbo_ms.id() : m_fbo.id());
	glViewport(0, 0, content_size.x, content_size.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Do the rendering
	m_renderer->draw(scene, camera, bu::to_vec2(content_size));	
	after_render();
	
	// If MSAA is enabled, blit to the usual framebuffer
	if (m_samples)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo.id());
		glDisable(GL_DEPTH_TEST);
		glUseProgram(m_downsample_program->id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_color_ms_tex->id());
		glUniform1i(m_downsample_program->get_uniform_location("tex"), 0);
		glUniform1i(m_downsample_program->get_uniform_location("samples"), m_samples);
		glUniform2i(m_downsample_program->get_uniform_location("size"), content_size.x, content_size.y);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}
	
	// Change back to the main FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	GLuint id = m_color_tex->id();
	ImVec2 tl{cursor_pos.x, cursor_pos.y};
	ImVec2 br{cursor_pos.x + content_size.x, cursor_pos.y + content_size.y};
	ImVec2 uv{content_size.x / m_fbo_tex_size.x, content_size.y / m_fbo_tex_size.y};
	ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(id), tl, br, ImVec2(0, uv.y), ImVec2(uv.x, 0));
	
	after();
}

rendered_view_window::rendered_view_window(bu::bunsen_editor &editor, int samples) :
	window("3D View", ImGuiWindowFlags_MenuBar),
	rendered_view(samples),
	m_editor(editor)
{
	m_renderer = std::make_unique<bu::preview_renderer>(m_editor.preview_ctx);
}

void rendered_view_window::update()
{
	if (m_renderer)
		m_renderer->update();
}

void rendered_view_window::draw()
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(10000, 10000));
	ImVec2 content_size = ImGui::GetContentRegionAvail();

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Renderers"))
		{
			if (ImGui::MenuItem("Preview"))
				m_renderer = std::make_unique<bu::preview_renderer>(m_editor.preview_ctx);

			if (ImGui::MenuItem("Basic Preview"))
				m_renderer = std::make_unique<bu::basic_preview_renderer>(m_editor.basic_preview_ctx);

			if (ImGui::MenuItem("Albedo"))
				m_renderer = std::make_unique<bu::albedo_renderer>(m_editor.albedo_ctx);

			if (ImGui::MenuItem("Path tracing"))
				m_renderer = std::make_unique<bu::rt_renderer>(m_editor.rt_ctx);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("MSAA"))
		{
			bool x0 = m_samples == 0;
			bool x2 = m_samples == 2;
			bool x4 = m_samples == 4;
			bool x8 = m_samples == 8;
			bool x16 = m_samples == 16;
			int new_msaa = -1;

			if (ImGui::MenuItem("Disabled", nullptr, x0))
				new_msaa = 0;

			if (ImGui::MenuItem("x2", nullptr, x2))
				new_msaa = 2;

			if (ImGui::MenuItem("x4", nullptr, x4))
				new_msaa = 4;
			
			if (ImGui::MenuItem("x8", nullptr, x8))
				new_msaa = 8;

			if (ImGui::MenuItem("x16", nullptr, x16))
				new_msaa = 16;
			
			if (new_msaa >= 0)
				set_content_size(bu::to_vec2(content_size), new_msaa);

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if (m_editor.scene && content_size.x > 0 && content_size.y > 0)
	{
		auto pos = bu::to_vec2(ImGui::GetWindowPos());
		auto min = pos + bu::to_vec2(ImGui::GetWindowContentRegionMin());
		auto max = pos + bu::to_vec2(ImGui::GetWindowContentRegionMax());
		auto click_pos = bu::to_vec2(ImGui::GetIO().MouseClickedPos[2]);
		bool click_inside = glm::all(glm::lessThanEqual(min, click_pos)) && glm::all(glm::lessThan(click_pos, max));
		bool hovered = ImGui::IsWindowHovered();
		bool transform_pending = m_layout_ed.is_transform_pending();
		bool drag_from_inside = ImGui::IsMouseDragging(2) && click_inside;
		bool trap_mouse = transform_pending || drag_from_inside;

		if (hovered && trap_mouse)
			lock_mouse();
		
		if (!trap_mouse)
			release_mouse();

		if (hovered && can_lock_mouse())
		{
			bu::update_camera_orbiter_from_imgui(m_orbiter, ImGui::GetIO(), glm::vec2{1440, 960}, m_mouse_offset);
			m_layout_ed.update(m_editor.scene, m_camera, bu::to_vec2(content_size), m_mouse_offset, m_overlay);
		}

		m_orbiter.update_camera(m_camera);
		m_camera.aspect = content_size.x / content_size.y;

		rendered_view::draw(*m_editor.scene, m_camera);
		m_overlay.draw();
	}
	else
	{
		ImGui::TextWrapped("No scene???");
	}
}