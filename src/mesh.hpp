#pragma once
#include <memory>
#include <vector>
#include "gl/gl.hpp"
#include "material.hpp"

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

}