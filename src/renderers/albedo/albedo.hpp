#pragma once
#include <memory>
#include "../../renderer.hpp"

namespace bu {

struct albedo_context
{

};

/**
	\brief Albedo is (will be) a bit more sophisticated OpenGL PBR renderer

	Hopefully it will do:
		- PBR rendering
		- shadow mapping
		- environment reflections
		- simple fog
*/
class albedo_renderer : public bu::renderer
{
public:
	albedo_renderer(std::shared_ptr<albedo_context> &context);
	void draw(const bu::scene &scene, const bu::camera &camera, const glm::ivec2 &viewport_size) override;

private:
	std::shared_ptr<albedo_context> m_context;
};

}