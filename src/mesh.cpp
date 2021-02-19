#include "mesh.hpp"
#include "utils.hpp"

using bu::mesh_gl_buffers;
using bu::mesh_data;

void mesh_gl_buffers::buffer_mesh(const mesh_data &m)
{
	auto vertex_count = m.positions.size();
	auto vertex_size = 3 + 3 + 2;
	
	std::vector<float> data(vertex_count * vertex_size);

	for (auto i = 0u; i < vertex_count; i++)
	{
		data[i * vertex_size + 0] = m.positions[i].x;
		data[i * vertex_size + 1] = m.positions[i].y;
		data[i * vertex_size + 2] = m.positions[i].z;

		// Normal (up if missing)
		glm::vec3 N;
		if (i < m.normals.size())
			N = m.normals[i];
		else
			N = glm::vec3{0, 1, 0};

		data[i * vertex_size + 3] = N.x;
		data[i * vertex_size + 4] = N.y;
		data[i * vertex_size + 5] = N.z;

		// UVs
		glm::vec2 uv;
		if (i < m.uvs.size())
			uv = m.uvs[i];
		else
			uv = glm::vec2{0};

		data[i * vertex_size + 6] = uv.x;
		data[i * vertex_size + 7] = uv.y;
	}

	glNamedBufferStorage(vertex_buffer.id(), bu::vector_size(data), data.data(), GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(index_buffer.id(), bu::vector_size(m.indices), m.indices.data(), GL_DYNAMIC_STORAGE_BIT);
}

void mesh_data::buffer()
{
	gl_buffers = std::make_unique<mesh_gl_buffers>();
	gl_buffers->buffer_mesh(*this);
}

void mesh_data::unbuffer()
{
	gl_buffers.reset();
}