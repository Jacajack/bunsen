#pragma once
#include <set>
#include <memory>
#include "../scene.hpp"
#include "../camera.hpp"
#include "imgui_overlay.hpp"
#include "scene_selection.hpp"

namespace bu {

/**
	\brief Layout editor is responsible for editing scene layout based on user input
*/
struct layout_editor
{
	enum class action_state
	{
		IDLE,
		
		GRAB,
		GRAB_X,
		GRAB_Y,
		GRAB_Z,

		ROTATE,
		ROTATE_X,
		ROTATE_Y,
		ROTATE_Z,

		SCALE,
	};

	void update(const bu::input_event_queue &input, const bu::scene_selection &selection, const bu::camera &cam, const glm::vec2 &viewport_size, bu::imgui_overlay &overlay);
	void start(const bu::input_event_queue &input, const bu::scene_selection &selection, action_state new_action);
	void apply();
	void abort();
	bool is_transform_pending();

	action_state state = action_state::IDLE;
	std::vector<std::shared_ptr<bu::transform_node>> transform_nodes;
	glm::mat4 transform_matrix;
	glm::vec3 transform_origin;
	glm::vec2 mouse_origin;
};

}