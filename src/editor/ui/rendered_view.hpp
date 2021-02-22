#pragma once 
#include <memory>
#include "window.hpp"
#include "../../renderer.hpp"
#include "../../renderers/preview/preview.hpp"
#include "../../scene.hpp"
#include "../../gl/gl.hpp"
#include "../layout_editor.hpp"
#include "../editor.hpp"

namespace bu::ui {

/**
	\brief Responsible for providing rendered view inside an ImGui window
		where draw() is called
*/
class rendered_view
{
public:
	rendered_view(int samples = 0);
	void draw(const bu::scene &scene, const bu::camera &camera);

protected:
	std::unique_ptr<bu::renderer> m_renderer;
	std::unique_ptr<bu::gl_texture> m_color_tex;
	std::unique_ptr<bu::gl_texture> m_depth_stencil_tex;
	
	bu::gl_fbo m_fbo;
	glm::ivec2 m_fbo_tex_size;
	int m_samples;

	void new_texture_storage(int w, int h, int samples = 0);
	virtual void before() {};
	virtual void after() {};
	virtual void after_render() {};
};

class rendered_view_window : public bu::ui::window, protected rendered_view
{
public:
	rendered_view_window(bu::bunsen_editor &editor, int samples = 0);
	void draw() override;

protected:
	bu::imgui_overlay m_overlay;
	bu::layout_editor m_layout_ed;
	bu::camera_orbiter m_orbiter;
	bu::camera m_camera;
	bu::bunsen_editor &m_editor;
};

}