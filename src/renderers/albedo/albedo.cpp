#include "albedo.hpp"

using bu::albedo_context;
using bu::albedo_renderer;

albedo_renderer::albedo_renderer(std::shared_ptr<albedo_context> &context) :
	m_context(std::move(context))
{
}

void albedo_renderer::draw(const bu::scene &scene, const bu::camera &camera, const glm::vec2 &viewport_size)
{
}