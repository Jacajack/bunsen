#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include "camera.hpp"
#include "gl/gl.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace br {

struct mesh;

/**
	\brief Equivalent of \ref mesh stored in GPU buffers.
	
	Storage format defined by this structure shall be consistent
	across entire application. That is:
		- One buffer for interleaved vertex data:
			- 3 x float - position
			- 3 x float - normal
			- 2 x float - uvs
		- Index buffer (unsigned int)
*/
struct mesh_gpu_buffers
{
	gl_buffer vertex_buffer;
	gl_buffer index_buffer;

	void buffer_mesh(const mesh &m);
};

struct mesh
{
	std::vector<glm::vec3> vertex_positions;
	std::vector<glm::vec3> vertex_normals;
	std::vector<glm::vec3> vertex_uvs;
	std::vector<unsigned int> indices;

	std::unique_ptr<mesh_gpu_buffers> gpu_buffers;
	void buffer_mesh();
	void unbuffer_mesh();
};

struct matrix_transform
{
	glm::vec3 scale = glm::vec3{1.f};
	glm::vec3 rotation = glm::vec3{0.f};
	glm::vec3 position = glm::vec3{0.f};

	glm::mat4 get_matrix() const
	{
		return glm::translate(glm::scale(glm::orientate4(rotation), scale), position);
	}
};

/**
	Object links mesh data with a matrix transform applied to it
*/
struct scene_object
{
	std::string name;
	matrix_transform transform;	
	int mesh_id;
};

struct scene
{
	std::vector<mesh> meshes;
	std::vector<scene_object> objects;
	std::vector<camera> cameras;
};

}