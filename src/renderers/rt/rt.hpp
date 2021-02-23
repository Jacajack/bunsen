#pragma once
#include <memory>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"

namespace bu {

struct rt_context
{
	std::unique_ptr<bu::shader_program> display_sampled_image;
};

class rt_renderer : public bu::renderer
{
public:
	rt_renderer(std::shared_ptr<rt_context> context);
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size) override;

private:
	std::shared_ptr<rt_context> m_context;
	std::unique_ptr<bu::gl_texture> result_tex;
};

}