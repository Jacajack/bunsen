#pragma once

#include <memory>
#include <set>
#include "../../gl/shader.hpp"
#include "../../scene.hpp"
#include "../../camera.hpp"

namespace bu {

class preview_renderer
{
public:
	preview_renderer();
	void draw(bu::scene &scene, const bu::camera &camera, const std::set<std::shared_ptr<bu::scene_node>> &selection);

private:
	std::unique_ptr<bu::shader_program> program;
	std::unique_ptr<bu::shader_program> grid_program;
	std::unique_ptr<bu::shader_program> outline_program;
	gl_vertex_array vao;
	gl_vertex_array vao_2d;
};

}