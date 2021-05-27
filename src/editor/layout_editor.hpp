#pragma once
#include <set>
#include <memory>
#include "../camera.hpp"
#include "imgui_overlay.hpp"

namespace bu {

class transform_node;
class scene;

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

	void update(std::shared_ptr<bu::scene> scene, const bu::camera &cam, const glm::vec2 &viewport_size, const glm::vec2 &mouse_offset, bu::imgui_overlay &overlay);
	void start(std::shared_ptr<bu::scene> scene, const glm::vec2 &mouse_offset, action_state new_action);
	void apply();
	void abort();
	bool is_transform_pending() const;

	action_state state = action_state::IDLE;
	std::shared_ptr<bu::scene> scene_ptr;
	std::vector<std::shared_ptr<bu::transform_node>> transform_nodes;
	glm::mat4 transform_matrix;
	glm::vec3 transform_origin;
	glm::vec2 mouse_origin;
};

}