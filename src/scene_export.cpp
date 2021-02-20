#include <set>
#include "scene_export.hpp"
#include "scene.hpp"
#include "log.hpp"
using namespace std::string_literals;

static nlohmann::json export_vec3(const glm::vec3 &v)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_mat4(const glm::mat4 &mat)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_light(const bu::model &light)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_material(const bu::model &model)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_mesh(const bu::model &model)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_model(const bu::model &model)
{
	LOG_WARNING << __PRETTY_FUNCTION__ << " is not implemented!";
	return {};
}

static nlohmann::json export_scene_node(const bu::scene_node &node)
{
	using json = nlohmann::json;
	json j;

	j["_type"] = "scene_node";

	// Export children
	auto &children = node.get_children();
	j["children"] = json::array();
	for (auto i = 0u; i < children.size(); i++)
		j["children"][i] = export_scene_node(*children[i]);

	return j;
}

nlohmann::json bu::export_scene(const bu::scene &scene)
{
	nlohmann::json j;
	j["_type"] = "scene";
	j["root_node"] = export_scene_node(*scene.root_node);
	return j;
}