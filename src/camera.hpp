#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

struct camera_orbiter
{
	glm::vec3 focus = {0, 0, 0};
	glm::vec3 focus_delta = {0, 0, 0};
	glm::vec2 angle = {0, 0};
	glm::vec2 angle_delta = {0, 0};
	float distance = 3;
	float distance_delta = 0;

	glm::vec2 spin_speed = 2.f * glm::pi<float>() * glm::vec2{2, 1};
	glm::vec2 translation_speed = glm::vec2{1.5f};
	float zoom_speed = 10.f;

	void spin(const glm::vec2 &d);
	void end_spin(bool cancel = false);

	void translate(const glm::vec2 &d);
	void end_translate(bool cancel = false);

	void zoom(const glm::vec2 &d);
	void end_zoom(bool cancel = false);

	void update_camera(camera &cam) const;
};

struct mouse_event_generator;
void update_camera_orbiter_from_mouse(camera_orbiter &orbiter, const br::mouse_event_generator &mouse, const glm::ivec2 &window_size);

}