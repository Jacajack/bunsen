#pragma once
#include "scene.hpp"
#include "camera.hpp"

namespace bu {

struct renderer 
{
	virtual ~renderer() = default;
	virtual void draw(const bu::scene &scene, const bu::camera &camera) = 0;
};

}