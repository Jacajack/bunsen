#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

using br::camera;

void camera::look_at(const glm::vec3 &what)
{
	direction = glm::normalize(what - position);
}

glm::mat4 camera::get_view_matrix() const
{
	return glm::lookAt(
		position,
		position + direction,
		up
	);
}

glm::mat4 camera::get_projection_matrix() const
{
	return glm::perspective(fov, aspect, near, far);
}