#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using br::camera;
using br::camera_orbiter;

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

void camera_orbiter::spin(const glm::vec2 &d)
{
	angle_delta = spin_speed * d * glm::vec2{1, -1};
}

void camera_orbiter::end_spin(bool cancel)
{
	if (!cancel)
		angle += angle_delta;
	angle_delta = glm::vec2{0.f};
}

void camera_orbiter::translate(const glm::vec2 &d)
{
	float yaw = angle.x + angle_delta.x;
	float pitch = angle.y + angle_delta.y;

	// Camera position on the sphere
	glm::vec3 cps = glm::max(distance + distance_delta, 0.01f) * glm::vec3{
		std::cos(yaw) * std::cos(pitch),
		std::sin(pitch),
		std::sin(yaw) * std::cos(pitch)
	};

	// Tangent to the horizontal circle
	glm::vec3 right = glm::vec3{
		-std::sin(yaw),
		0,
		std::cos(yaw)
	};

	// Camera forward direction
	auto forward = glm::normalize(-cps);
	auto up = glm::cross(right, forward);

	focus_delta = (distance + distance_delta) * glm::mat3{-right, up, forward} * glm::vec3{d * translation_speed, 0.f};
}

void camera_orbiter::end_translate(bool cancel)
{
	if (!cancel)
		focus += focus_delta;
	focus_delta = glm::vec3{0.f};
}

void camera_orbiter::zoom(const glm::vec2 &d)
{
	distance_delta = distance * (std::pow(1.2f, d.y * zoom_speed) - 1.f);
}

void camera_orbiter::end_zoom(bool cancel)
{
	if (!cancel)
		distance += distance_delta;
	distance_delta = 0.f;
}

void camera_orbiter::update_camera(camera &cam) const
{
	float yaw = angle.x + angle_delta.x;
	float pitch = angle.y + angle_delta.y;

	// Camera position on the sphere
	glm::vec3 cps = glm::max(distance + distance_delta, 0.01f) * glm::vec3{
		std::cos(yaw) * std::cos(pitch),
		std::sin(pitch),
		std::sin(yaw) * std::cos(pitch)
	};

	// Camera forward direction
	auto forward = glm::normalize(-cps);

	// Tangent to the horizontal circle
	glm::vec3 right = glm::vec3{
		-std::sin(yaw),
		0,
		std::cos(yaw)
	};

	cam.position = cps + focus + focus_delta;
	cam.direction = forward;
	cam.up = glm::cross(right, forward);
}