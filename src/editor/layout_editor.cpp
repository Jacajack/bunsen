#include "layout_editor.hpp"
#include "../log.hpp"
#include "../input.hpp"
#include <cmath>
#include <glm/gtx/transform.hpp>

using bu::layout_editor;

static float ray_plane_intersection(const glm::vec3 &ro, const glm::vec3 &rd, const glm::vec3 &po, const glm::vec3 &n)
{
	float n_dot_dir = glm::dot(n, rd);
	if (n_dot_dir != 0)
	{
		return glm::dot(n, po - ro) / n_dot_dir;
	}
	else return HUGE_VALF;
}

void layout_editor::update(
	const bu::input_event_queue &input,
	const std::list<std::weak_ptr<bu::scene_node>> &selection,
	const bu::camera &cam,
	const glm::vec2 &viewport_size,
	bu::imgui_overlay &overlay)
{
	auto was_pressed = [&input](int key)
	{
		for (auto &ev : input.get_queue())
			if (ev.type == input_event_type::KEYBOARD && ev.action == GLFW_PRESS && ev.key == key)
				return true;
		return false;
	};

	auto was_clicked = [&input](int button)
	{
		for (auto &ev : input.get_queue())
			if (ev.type == input_event_type::MOUSE_BUTTON && ev.action == GLFW_PRESS && ev.key == button)
				return true;
		return false;
	};

	auto normalize_mouse_pos = [&cam, &viewport_size](const glm::vec2 &pos)
	{
		return (pos / viewport_size * 2.f - 1.f) * glm::vec2{1, -1};
	};

	// Converts mouse pos in pixels to ray direction
	auto mouse_raydir = [&cam, &normalize_mouse_pos](const glm::vec2 &pos)
	{
		auto npos = normalize_mouse_pos(pos);
		float near = cam.near;
		float half_height = near * glm::tan(cam.fov * 0.5f);
		float half_width = half_height * cam.aspect;
		auto fwd = glm::normalize(cam.direction);
		auto right = glm::normalize(glm::cross(fwd, cam.up));
		auto up = glm::normalize(glm::cross(right, fwd));
		return glm::normalize(
			fwd * near
			+ right * half_width * npos.x
			+ up * half_height * npos.y
		);
	};

	// Mouse position and delta in pixels
	auto mouse_pos = input.get_position();
	auto mouse_delta = mouse_pos - mouse_origin;

	// auto mr = mouse_raydir(mouse_pos);
	// auto vp_mat = cam.get_projection_matrix() * cam.get_view_matrix();
	// overlay.add_3d_line(vp_mat * glm::vec4{cam.position + mr * 1.f, 1}, glm::vec4{0, 0, 0, 1});
	// overlay.add_3d_line(vp_mat * glm::vec4{cam.position + mr * 1.f, 1}, vp_mat * glm::vec4{0, 0, 0, 1});

	switch (state)
	{
		case action_state::IDLE:
			if (was_pressed(GLFW_KEY_G))
				start(input, selection, action_state::GRAB);

			if (was_pressed(GLFW_KEY_R))
				start(input, selection, action_state::ROTATE);
			break;

		case action_state::GRAB:
		{
			auto pn = -glm::normalize(cam.direction);
			auto po = transform_origin;
			auto ro = cam.position;
			auto rd_origin = mouse_raydir(mouse_origin);
			auto rd_current = mouse_raydir(mouse_pos);
			auto t_origin = ray_plane_intersection(ro, rd_origin, po, pn);
			auto t_current = ray_plane_intersection(ro, rd_current, po, pn);
			auto origin_intersect = ro + rd_origin * t_origin;
			auto current_intersect = ro + rd_current * t_current;
			auto delta = current_intersect - origin_intersect;
			transform_matrix = glm::translate(glm::mat4{1.f}, delta);
	
			// Axes
			auto vp_mat = cam.get_projection_matrix() * cam.get_view_matrix();
			auto orig = transform_matrix * glm::vec4{transform_origin, 1};
			auto proj = cam.get_projection_matrix();
			glm::vec4 color_x{1.0, 0.4, 0.4, 1.0};
			glm::vec4 color_y{0.4, 1.0, 0.4, 1.0};
			glm::vec4 color_z{0.4, 0.4, 1.0, 1.0};
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{1, 0, 0, 0},  color_x);
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{-1, 0, 0, 0}, color_x);
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{0, 1, 0, 0},  color_y);
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{0, -1, 0, 0}, color_y);
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{0, 0, 1, 0},  color_z);
			overlay.add_3d_line(vp_mat * orig, vp_mat * glm::vec4{0, 0, -1, 0}, color_z);

			if (was_clicked(0))
				apply();

			if (was_clicked(1) || was_pressed(GLFW_KEY_G) || was_pressed(GLFW_KEY_ESCAPE))
				abort();
			break;
		}

		case action_state::ROTATE:
		{
			auto to = cam.get_projection_matrix() * cam.get_view_matrix() * glm::vec4{transform_origin, 1};
			auto to_projected = glm::vec2{to / to.w};
			auto delta_1 = normalize_mouse_pos(mouse_origin) - to_projected;
			auto delta_2 = normalize_mouse_pos(mouse_pos) - to_projected;
			auto angle_1 = std::atan2(delta_1.y, delta_1.x);
			auto angle_2 = std::atan2(delta_2.y, delta_2.x);

			transform_matrix = glm::translate(glm::mat4{1.f}, transform_origin) * glm::rotate(angle_1 - angle_2, cam.direction) * glm::translate(glm::mat4{1.f}, -transform_origin);
			overlay.add_line(to_projected, normalize_mouse_pos(mouse_pos));

			if (was_clicked(0))
				apply();

			if (was_clicked(1) || was_pressed(GLFW_KEY_R) || was_pressed(GLFW_KEY_ESCAPE))
				abort();
			
			break;
		}

		default:
			abort();
			break;		
	}
}

void layout_editor::start(
	const bu::input_event_queue &input,
	const std::list<std::weak_ptr<bu::scene_node>> &selection,
	action_state new_action)
{
	if (is_transform_pending())
		abort();
	state = new_action;

	// TODO compute LCA nodes

	auto origin = glm::vec3{0.f};
	int node_count = 0;

	transform_matrix = glm::mat4{1.f};

	try
	{
		for (auto &sn : selection)
		{
			if (auto node = sn.lock())
			{
				// Take node's origin into account
				origin += glm::vec3(node->get_final_transform() * glm::vec4{0, 0, 0, 1});
				node_count++;
				
				// Insert transform node
				std::shared_ptr<bu::scene_node> parent{node->get_parent()};
				auto tn = std::make_shared<bu::transform_node>(&transform_matrix);
				parent->add_child(tn);
				tn->add_child(node);
				transform_nodes.push_back(std::move(tn));
			}
		}
	}
	catch (const std::exception &ex)
	{
		LOG_WARNING << "Failed to initate scene transform! (" << ex.what() << ")";
		abort();
	}

	// Compute transform origin point
	if (node_count)
		transform_origin = origin / float(node_count);
	else
		transform_origin = glm::vec3{0.f};

	// Store mouse position
	mouse_origin = input.get_position();

}

void layout_editor::apply()
{
	for (const auto &n : transform_nodes)
		n->apply();
	transform_nodes.clear();
	state = action_state::IDLE;
}

void layout_editor::abort()
{
	for (const auto &n : transform_nodes)
		n->dissolve();
	transform_nodes.clear();
	state = action_state::IDLE;
}

bool layout_editor::is_transform_pending()
{
	return state != action_state::IDLE;
}
