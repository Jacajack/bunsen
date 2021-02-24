#pragma once
#include "scene.hpp"
#include "camera.hpp"
#include <glm/glm.hpp>

namespace bu {

struct renderer 
{
	virtual ~renderer() = default;
	virtual void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewwport_size) = 0;
	virtual void update() {};
};

}