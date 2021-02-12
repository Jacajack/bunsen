#pragma once

#include <memory>
#include "../gl/shader.hpp"
#include "../scene.hpp"
#include "../camera.hpp"

namespace bu {

class preview_renderer
{
public:
	preview_renderer();
	void draw(bu::scene &scene, const bu::camera &camera, const scene_node *selected_node = nullptr);

private:
	std::unique_ptr<bu::shader_program> program;
	std::unique_ptr<bu::shader_program> grid_program;
	gl_vertex_array vao;
};

}