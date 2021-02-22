#include "rendered_view.hpp"
#include <imgui.h>
#include "../../log.hpp"

using bu::ui::rendered_view;
using bu::ui::rendered_view_window;

rendered_view::rendered_view(int samples) :
	m_renderer(std::make_unique<bu::preview_renderer>()),
	m_samples(samples)
{
	new_texture_storage(1024, 1024, m_samples);
}

void rendered_view::new_texture_storage(int w, int h, int samples)
{
	LOG_DEBUG << "Creating new " << w << "x" << h << " (" << samples << " samples" << ") texture storage for rendered_view";
	m_fbo_tex_size = glm::ivec2(w, h);

	// Color texture
	std::unique_ptr<bu::gl_texture> new_color_tex;
	if (samples)
	{
		// glTexStorage2DMultisample() is GL 4.3 and higher
		new_color_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D_MULTISAMPLE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, new_color_tex->id());
		glTexImage2DMultisample(GL_TEXTURE_2D, samples, GL_RGBA8, w, h, GL_FALSE);
	}
	else
	{
		// Sadly, we can't use glTextureStorage2D() because it's from GL 4.5
		new_color_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, new_color_tex->id());
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
	}
	m_color_tex = std::move(new_color_tex);

	// Depth-stencil
	auto new_depth_stencil_tex = std::make_unique<bu::gl_texture>(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, new_depth_stencil_tex->id());
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH32F_STENCIL8, w, h);
	m_depth_stencil_tex = std::move(new_depth_stencil_tex);

	// Bind textures to FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo.id());
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_tex->id(), 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth_stencil_tex->id(), 0);
}

void rendered_view::draw(const bu::scene &scene, const bu::camera &camera)
{
	ImVec2 content_size = ImGui::GetContentRegionAvail();
	ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

	// Update texture size
	if (content_size.x != m_fbo_tex_size.x || content_size.y != m_fbo_tex_size.y)
	{
		int w = std::exp2(std::ceil(std::log2(content_size.x)));
		int h = std::exp2(std::ceil(std::log2(content_size.y)));
		if (m_fbo_tex_size.x != w || m_fbo_tex_size.y != h)
			new_texture_storage(w, h, m_samples);
	}

	bu::imgui_overlay overlay;

	before();

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo.id());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, content_size.x, content_size.y);
	m_renderer->draw(scene, camera);
	after_render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	GLuint id = m_color_tex->id();
	ImVec2 tl{cursor_pos.x, cursor_pos.y};
	ImVec2 br{cursor_pos.x + content_size.x, cursor_pos.y + content_size.y};
	ImVec2 uv{content_size.x / m_fbo_tex_size.x, content_size.y / m_fbo_tex_size.y};
	ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<void*>(id), tl, br, ImVec2(0, uv.y), ImVec2(uv.x, 0));
	
	after();

	overlay.draw();
}

rendered_view_window::rendered_view_window(bu::bunsen_editor &editor, int samples) :
	window("3D View"),
	rendered_view(samples),
	m_editor(editor)
{}

void rendered_view_window::draw()
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(10000, 10000));
	ImVec2 content_size = ImGui::GetContentRegionAvail();

	if (m_editor.scene && content_size.x > 0 && content_size.y > 0)
	{
		auto size = bu::to_vec2(ImGui::GetWindowSize());
		auto pos = bu::to_vec2(ImGui::GetWindowPos());
		auto click_pos = bu::to_vec2(ImGui::GetIO().MouseClickedPos[2]);
		bool click_inside = glm::all(glm::lessThanEqual(pos, click_pos)) && glm::all(glm::lessThan(click_pos, pos + size));
		bool hovered = ImGui::IsWindowHovered();
		bool transform_pending = m_layout_ed.is_transform_pending();
		bool drag_from_inside = ImGui::IsMouseDragging(2) && click_inside;
		bool trap_mouse = transform_pending || drag_from_inside;

		if (trap_mouse && !hovered)
		{
			LOG_DEBUG << "Should roll mouse";
		}

		if (hovered)
		{
			bu::update_camera_orbiter_from_imgui(m_orbiter, ImGui::GetIO());
			m_layout_ed.update(*m_editor.scene, m_camera, bu::to_vec2(content_size), m_overlay);
			
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