#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <functional>
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

	Nodes can be transformed relative to their parents. They can contain and own
	light sources and cameras. 
	
*/
struct scene_node
{
	std::string name;
	glm::mat4 transform = glm::mat4{1.f};
	bool visible = true;
	
	std::vector<scene_node> children;
	std::vector<mesh> meshes;
	std::vector<camera> cameras;
	std::vector<light> lights;

	scene_node &find_node(const std::string &name) const;

	template <typename T>
	void foreach_node(std::function<T> f)
	{
		f(*this);
		for (auto &c : children)
			c.foreach_node(f);
	}
};

/**
	\brief The scene.
*/
struct scene
{
	scene();

	scene_node root_node;
};

}