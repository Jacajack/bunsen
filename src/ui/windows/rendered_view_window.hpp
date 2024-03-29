#pragma once 
#include <memory>
#include "ui/window.hpp"
#include "camera.hpp"
#include "gl/gl.hpp"
#include "ui/imgui_overlay.hpp"

namespace bu {
struct renderer;
class shader_program;
struct bunsen_editor;
struct scene;
}

namespace bu::ui {

/**
	\brief Responsible for providing rendered view inside an ImGui window
		where draw() is called
*/
class rendered_view
{
public:
	rendered_view(int samples = -1);
	void draw(const bu::scene &scene, const bu::camera &camera);

protected:
	std::unique_ptr<bu::renderer> m_renderer;

	std::unique_ptr<bu::shader_program> m_downsample_program;

	std::unique_ptr<bu::gl_texture> m_color_tex;
	std::unique_ptr<bu::gl_texture> m_color_ms_tex;
	std::unique_ptr<bu::gl_texture> m_depth_stencil_tex;
	std::unique_ptr<bu::gl_texture> m_depth_stencil_ms_tex;
	
	bu::gl_fbo m_fbo;
	bu::gl_fbo m_fbo_ms;
	glm::ivec2 m_fbo_tex_size = {0, 0};
	int m_samples;

	void new_texture_storage(int w, int h, int samples = 0);
	void set_content_size(const glm::vec2 &content_size, int samples = -1);
	virtual void before() {};
	virtual void after() {};
	virtual void after_render() {};
};

class rendered_view_window : public bu::ui::window, protected rendered_view
{
public:
	rendered_view_window(bu::bunsen_editor &editor, int samples = -1, std::shared_ptr<bu::event_bus> bus = {});
	void update() override;
	void draw() override;

protected:
	bu::imgui_overlay m_overlay;
	bu::camera_orbiter m_orbiter;
	bu::camera m_camera;
	bu::bunsen_editor &m_editor;

	bool m_camera_drag_pending = false;
};

}