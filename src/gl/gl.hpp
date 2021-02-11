#pragma once

/**
	\file gl.hpp Provides right order of OpenGL-related includes and RAII
		wrappers for GL objects (TODO)
*/

#include <utility>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace br {

void gl_debug_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *user);

class gl_object
{
public:
	GLuint id() const
	{
		return m_id;
	}

	gl_object(const gl_object &) = delete;
	gl_object &operator=(const gl_object &) = delete;

	gl_object(gl_object &&src) noexcept :
		m_id(src.m_id)
	{
		src.m_id = 0;
	}

	gl_object &operator=(gl_object &&rhs) noexcept
	{
		std::swap(m_id, rhs.m_id);
		return *this;
	}

protected:
	GLuint m_id = 0;

	gl_object() = default;
	~gl_object() = default;
};

class gl_buffer : public gl_object
{
public:
	gl_buffer() 
	{
		glCreateBuffers(1, &m_id);
	}

	~gl_buffer()
	{
		glDeleteBuffers(1, &m_id);
	}

	gl_buffer(gl_buffer &&) = default;
	gl_buffer &operator=(gl_buffer &&) = default;
};

class gl_vertex_array : public gl_object
{
public:
	gl_vertex_array() 
	{
		glCreateVertexArrays(1, &m_id);
	}

	~gl_vertex_array()
	{
		glDeleteVertexArrays(1, &m_id);
	}

	gl_vertex_array(gl_vertex_array &&) = default;
	gl_vertex_array &operator=(gl_vertex_array &&) = default;
};

class gl_shader : public gl_object
{
public:
	gl_shader(GLenum type) 
	{
		m_id = glCreateShader(type);
	}

	~gl_shader()
	{
		glDeleteShader(m_id);
	}

	gl_shader(gl_shader &&) = default;
	gl_shader &operator=(gl_shader &&) = default;
};

class gl_program : public gl_object
{
public:
	gl_program() 
	{
		m_id = glCreateProgram();
	}

	~gl_program()
	{
		glDeleteProgram(m_id);
	}

	gl_program(gl_program &&) = default;
	gl_program &operator=(gl_program &&) = default;
};

}