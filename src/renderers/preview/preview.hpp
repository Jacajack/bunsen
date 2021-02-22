#pragma once
#include <memory>
#include <set>
#include <glm/glm.hpp>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"
#include "../../renderer.hpp"

namespace bu {

class preview_renderer : public bu::renderer
{
public:
	preview_renderer();
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size) override;

private:
	std::unique_ptr<bu::shader_program> program;
	std::unique_ptr<bu::shader_program> grid_program;
	std::unique_ptr<bu::shader_program> outline_program;
	std::unique_ptr<bu::shader_program> light_program;
	gl_vertex_array vao;
	gl_vertex_array vao_2d;
};

}