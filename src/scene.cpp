#include "scene.hpp"
#include "utils.hpp"

using br::mesh_gpu_buffers;
using br::mesh;

void mesh_gpu_buffers::buffer_mesh(const mesh &m)
{
	auto vertex_count = m.vertex_positions.size();
	auto vertex_size = 3 + 3 + 2;
	
	std::vector<float> data(vertex_count * vertex_size);

	for (auto i = 0u; i < vertex_count; i++)
	{
		data[i * vertex_size + 0] = m.vertex_positions[i].x;
		data[i * vertex_size + 1] = m.vertex_positions[i].y;
		data[i * vertex_size + 2] = m.vertex_positions[i].z;

		// Normal (up if missing)
		glm::vec3 N;
		if (i < m.vertex_normals.size())
			N = m.vertex_normals[i];
		else
			N = glm::vec3{0, 1, 0};

		data[i * vertex_size + 3] = N.x;
		data[i * vertex_size + 4] = N.y;
		data[i * vertex_size + 5] = N.z;

		// UVs
		glm::vec2 uv;
		if (i < m.vertex_uvs.size())
			uv = m.vertex_uvs[i];
		else
			uv = glm::vec2{0};

		data[i * vertex_size + 6] = uv.x;
		data[i * vertex_size + 7] = uv.y;
	}

	glNamedBufferStorage(vertex_buffer.id(), br::vector_size(data), data.data(), GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(index_buffer.id(), br::vector_size(m.indices), m.indices.data(), GL_DYNAMIC_STORAGE_BIT);
}

void mesh::buffer_mesh()
{
	gpu_buffers = std::make_unique<mesh_gpu_buffers>();
	gpu_buffers->buffer_mesh(*this);
}

void mesh::unbuffer_mesh()
{
	gpu_buffers.reset();
}