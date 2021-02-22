#include "camera.hpp"
#include "input.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>

using bu::camera;
using bu::camera_orbiter;

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

void bu::update_camera_orbiter_from_mouse(camera_orbiter &orbiter, const bu::input_event_queue &inputs, const glm::vec2 &scale)
{
	auto normalize_mouse_pos = [scale](glm::vec2 pos)
	{
		return pos / scale;
	};

	// Orbiting camera control
	if (inputs.is_drag_pending(2))
	{
		auto ndelta = normalize_mouse_pos(inputs.get_drag_delta(2));
		int mods = inputs.get_drag_mods(2);
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl = mods & GLFW_MOD_CONTROL;
		if (!shift & !ctrl)
			orbiter.spin(ndelta);
		else if (ctrl)
			orbiter.zoom(ndelta);
		else 
			orbiter.translate(ndelta);
	}

	// Orbiting camera control - end drag
	for (const auto &ev : inputs.get_queue())
	{
		if (ev.type == bu::input_event_type::DRAG_END && ev.key == 2)
		{
			auto ndelta = normalize_mouse_pos(ev.position - ev.start_position);
			int mods = inputs.get_drag_mods(2);
			bool shift = mods & GLFW_MOD_SHIFT;
			bool ctrl = mods & GLFW_MOD_CONTROL;
			if (!shift & !ctrl)
			{
				orbiter.spin(ndelta);
				orbiter.end_spin();
			}
			else if (ctrl)
			{
				orbiter.zoom(ndelta);
				orbiter.end_zoom();
			}
			else
			{
				orbiter.translate(ndelta);
				orbiter.end_translate();
			}
		}
	}

	orbiter.distance *= std::pow(1.2, -inputs.get_scroll().y);
}

void bu::update_camera_orbiter_from_imgui(camera_orbiter &orbiter, const ImGuiIO &io, const glm::vec2 &scale)
{
	auto normalize_mouse_pos = [scale](glm::vec2 pos)
	{
		return pos / scale;
	};

	// Orbiting camera control
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
	{
		auto d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
		auto ndelta = normalize_mouse_pos(glm::vec2(d.x, d.y));
		bool shift = io.KeyShift;
		bool ctrl = io.KeyCtrl;
		if (!shift & !ctrl)
			orbiter.spin(ndelta);
		else if (ctrl)
			orbiter.zoom(ndelta);
		else 
			orbiter.translate(ndelta);
	}
	
	if (ImGui::IsMouseReleased(2))
	{
		auto d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
		auto ndelta = normalize_mouse_pos(glm::vec2(d.x, d.y));
		bool shift = io.KeyShift;
		bool ctrl = io.KeyCtrl;
		if (!shift & !ctrl)
		{
			orbiter.spin(ndelta);
			orbiter.end_spin();
		}
		else if (ctrl)
		{
			orbiter.zoom(ndelta);
			orbiter.end_zoom();
		}
		else
		{
			orbiter.translate(ndelta);
			orbiter.end_translate();
		}
	}

	orbiter.distance *= std::pow(1.2, -io.MouseWheel);
}