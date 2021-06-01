#include "node_widget.hpp"
#include <string>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/quaternion.hpp>
#include "scene.hpp"
using bu::ui::node_widget;

/**
	\brief Property editor for nodes (transforms, names)
	\todo Make transform UI affect the objects
*/
void node_widget::draw(std::shared_ptr<bu::scene_node> node)
{
	bool visible = node->is_visible();
	bool local_transform = node->get_transform_origin() == bu::scene_node::node_transform_origin::PARENT;
	
	// Name buffer
	const auto name_max_len = 32u;
	std::string name = node->get_name();
	std::vector<char> buf(std::max(name_max_len, static_cast<unsigned int>(name.size()) + 1));
	std::copy(name.begin(), name.end(), buf.begin());

	// Decompose transform
	glm::vec3 translate;
	glm::mat4 mat = node->get_transform();
	translate = mat[3];
	mat[3] = glm::vec4{0.f};
	glm::vec3 scale{glm::length(mat[0]), glm::length(mat[1]), glm::length(mat[2])};
	mat[0] /= scale.x;
	mat[1] /= scale.y;
	mat[2] /= scale.z;
	auto quat = glm::quat_cast(mat);

	// Reassemble matrix
	// quat = glm::normalize(quat);
	// mat = glm::translate(glm::scale(glm::mat4_cast(quat), scale), translate);

	// Name 
	ImGui::InputText("Name", buf.data(), name_max_len);
	ImGui::Dummy(ImVec2(0.f, 5.f));
	
	// Transforms (currently disabled)
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	ImGui::DragFloat3("Translation", &translate[0], 0.1f, 0, 0);
	ImGui::DragFloat4("Rotation", &quat[0], 0.1f, 0, 0);
	ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0, 0);
	ImGui::PopItemFlag();
	ImGui::PopStyleVar();
	ImGui::Dummy(ImVec2(0.f, 5.f));

	// Other attributes
	bool changed_local_transform = ImGui::Checkbox("Local transform", &local_transform);
	bool changed_visible = ImGui::Checkbox("Visible", &visible);
	
	ImGui::Dummy(ImVec2(0.f, 5.f));
	ImGui::Text("Node UID: %lu", node->uid());

	// Write back to the node
	if (changed_visible) node->set_visible(visible);
	if (changed_local_transform) node->set_transform_origin(local_transform ? bu::scene_node::node_transform_origin::PARENT : bu::scene_node::node_transform_origin::WORLD);
	// node->set_transform(mat);

	// Do not pass entire buffer to std::string{} because it contains terminating NULs
	auto len = std::strlen(buf.data());
	node->set_name(std::string{buf.begin(), buf.begin() + len});
}
