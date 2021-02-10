#pragma once

#include <glm/glm.hpp>

namespace br {

struct camera
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 up = glm::vec3{0, 1, 0};
	float aspect = 16.f / 9.f;
	float fov = glm::radians(60.f);
	float near = 0.01f;
	float far = 100.f;

	void look_at(const glm::vec3 &what);
	glm::mat4 get_view_matrix() const;
	glm::mat4 get_projection_matrix() const;
};

}