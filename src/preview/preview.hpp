#pragma once

#include <memory>
#include "../gl/shader.hpp"
#include "../scene.hpp"
#include "../camera.hpp"

namespace br {

class preview_renderer
{
public:
	preview_renderer();
	void draw(br::scene &scene, const br::camera &camera, const scene_node *selected_node = nullptr);

private:
	std::unique_ptr<br::shader_program> program;
	std::unique_ptr<br::shader_program> grid_program;
	gl_vertex_array vao;
};

}