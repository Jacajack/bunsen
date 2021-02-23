#include "rt.hpp"

using bu::rt_renderer;

rt_renderer::rt_renderer(std::shared_ptr<rt_context> context) :
	m_context(std::move(context))
{
}

void rt_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size)
{

}