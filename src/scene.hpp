#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <stack>
#include "camera.hpp"
#include "gl/gl.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace bu {

struct mesh_data;

/**
	\brief Equivalent of \ref mesh_data stored in GPU buffers.
	
	Storage format defined by this structure shall be consistent
	across entire application. That is:
		- One buffer for interleaved vertex data:
			- 3 x float - position
			- 3 x float - normal
			- 2 x float - uvs
		- Index buffer (unsigned int)
*/
struct mesh_gl_buffers
{
	gl_buffer vertex_buffer;
	gl_buffer index_buffer;

	void buffer_mesh(const mesh_data &m);
};

/**
	\brief Abstract surface material
*/
struct surface_material
{
};

/**
	\brief Abstract volume material
*/
struct volume_material
{
};

struct light
{
};

/**
	\brief Material data holds and owns shaders for both surface and volume shading
*/
struct material_data
{
	std::string name;
	std::unique_ptr<surface_material> surface;
	std::unique_ptr<volume_material> volume;
};

/**
	\brief Mesh data contains and owns purely geometrical vertex information

	The mesh data may include a pointer to a struct containing the same data
	in GL buffers.
*/
struct mesh_data
{
	std::string name;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> uvs;
	std::vector<unsigned int> indices;

	std::unique_ptr<mesh_gl_buffers> gl_buffers;
	void buffer();
	void unbuffer();
};

/**
	\brief Meshes link together mesh data (vertex stuff) and material data
		and can share this data with other meshes.
*/
struct mesh
{
	std::shared_ptr<mesh_data> data;
	std::shared_ptr<material_data> mat;
};

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
	scene_node();

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

	// Iterating
	dfs_iterator begin();
	dfs_iterator end();

	virtual ~scene_node();

protected:
	// Transform
	glm::mat4 m_transform = glm::mat4{1.f};
	node_transform_origin m_transform_origin = node_transform_origin::PARENT;


	std::string m_name;
	
	// Attributes
	bool m_visible = true;
	bool m_selected = false;

private:
	// Parenting
	std::weak_ptr<scene_node> m_parent;
	std::vector<std::shared_ptr<scene_node>> m_children;

	void remove_from_parent() noexcept;
	void set_parent(std::shared_ptr<scene_node> p);
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
	\brief Like scene_node, but cannot have children
*/
class scene_leaf : public scene_node
{
private:
		// Children-related functions
};

/**
	\brief A node for making transformations easier

	Transformations applied by this node behave differently when the origin is
	set to WORLD. The children are then transformed in world space but without
	affecting parent nodes.

	When apply() is called, the node
*/
class transform_node : public scene_node
{
public:
	void apply();

	glm::mat4 get_transform() const override;
};

struct mesh_node : public scene_leaf
{
	std::vector<mesh> meshes;
};

struct light_node : public scene_leaf
{
};

struct camera_node : public scene_leaf
{
};




/**
	\brief The scene.
*/
struct scene
{
	scene();

	std::shared_ptr<scene_node> root_node;
};

}