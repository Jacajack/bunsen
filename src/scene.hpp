#pragma once
#include <vector>
#include <string>
#include <memory>
#include <stack>

#include <glm/glm.hpp>

#include "world.hpp"
#include "model.hpp"
#include "light.hpp"
#include "camera.hpp"
#include "scene_selection.hpp"

namespace bu {

/**
	\brief Basic element of a scene.
*/
class scene_node : public std::enable_shared_from_this<scene_node>
{
public:
	enum class node_transform_origin
	{
		PARENT,
		WORLD
	};

	friend class dfs_iterator;
	class dfs_iterator;

	// Constructors
	scene_node() = default;
	scene_node(const scene_node &) = delete;
	scene_node &operator=(const scene_node &) = delete;
	
	// Move semantics
	scene_node(scene_node &&rhs);
	scene_node &operator=(scene_node &&src);

	// Perhaps move this to abstract base
	virtual std::shared_ptr<scene_node> clone(std::shared_ptr<scene_node> parent = {}) const;

	// Transforms
	virtual glm::mat4 get_transform() const;
	glm::mat4 get_final_transform() const;
	glm::mat4 get_transform_relative_to(const scene_node *what) const;
	void set_transform(const glm::mat4 &t);
	node_transform_origin get_transform_origin() const;
	void set_transform_origin(node_transform_origin o);

	// Parenting
	bool has_parent() const;
	bool is_orphan() const;
	std::weak_ptr<scene_node> get_parent();
	const std::weak_ptr<scene_node> get_parent() const;
	void add_child(std::shared_ptr<scene_node> c);
	std::shared_ptr<scene_node> remove_child(scene_node *c);
	void remove_from_parent() noexcept;
	void set_parent(std::shared_ptr<scene_node> p);
	void dissolve();

	// Iterating
	dfs_iterator begin();
	dfs_iterator end();
	const std::vector<std::shared_ptr<scene_node>> &get_children() const;

	// Properties
	const std::string &get_name() const;
	void set_name(const std::string &name);
	bool is_visible() const;
	void set_visible(bool v);

	virtual ~scene_node();

protected:
	// Transform
	glm::mat4 m_transform = glm::mat4{1.f};
	node_transform_origin m_transform_origin = node_transform_origin::PARENT;

	// Parenting
	std::weak_ptr<scene_node> m_parent;
	std::vector<std::shared_ptr<scene_node>> m_children;

	std::string m_name;
	
	// Attributes
	bool m_visible = true;

};

/**
	\brief Iterates over the scene in a DFS manner
*/
class scene_node::dfs_iterator
{
public:
	struct end_tag {};

	dfs_iterator(end_tag);
	dfs_iterator(scene_node &n);
	scene_node &operator*();
	dfs_iterator &operator++();
	bool operator==(const dfs_iterator &rhs) const;
	glm::mat4 get_transform();

private:
	std::vector<scene_node*> node_stack;
	std::vector<glm::mat4> transform_stack;
	int counter = 0;
};

/**
	\brief A node for making transformations easier

	Transformations applied by this node behave differently when the origin is
	set to WORLD. The children are then transformed in world space without
	affecting parent nodes or childrens' transform modes.

	When transform origin is set to PARENT the transform node behaves just
	like any other scene node.

	The transform nodes relative to WORLD are the only type of node that affect
	their children with transform origin set to WORLD.

	The transform node can contain a pointer to a transform matrix. If not null,
	this matrix will be used for the transform instead of the internal one.

	When apply() is called, the node applies the transform on its children.
	Simply dissolving the node won't apply the transform.
*/
class transform_node : public scene_node
{
public:
	transform_node(const glm::mat4 *ext_mat = nullptr);

	std::shared_ptr<scene_node> clone(std::shared_ptr<scene_node> parent = {}) const override;

	void apply();
	glm::mat4 get_transform() const override;
	const glm::mat4 &get_raw_transform() const;

	const glm::mat4 *transform_ptr;
};

/**
	\brief Node containing the actual objects on the scene
*/
struct model_node : public scene_node
{
	std::shared_ptr<scene_node> clone(std::shared_ptr<scene_node> parent = {}) const override;
	std::shared_ptr<bu::model> model;
};

struct light_node : public scene_node
{
	std::shared_ptr<scene_node> clone(std::shared_ptr<scene_node> parent = {}) const override;
	std::shared_ptr<bu::light> light;
};

struct camera_node : public scene_node
{
};

/**
	\brief The scene.
*/
struct scene
{
	scene();

	std::shared_ptr<scene_node> root_node;
	std::shared_ptr<bu::world> world;
	bu::scene_selection selection;
};

}