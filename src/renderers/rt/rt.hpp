#pragma once
#include <memory>
#include <set>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"

namespace bu {

class rt_renderer : public bu::renderer
{
public:
	rt_renderer();
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size) override;

private:
	std::unique_ptr<bu::gl_texture> result_tex;
	std::unique_ptr<bu::shader_program> display_sampled_image;
};

}