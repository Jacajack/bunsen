#include "shader.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>
#include "gl.hpp"

using br::gl_shader;
using br::gl_program;
using br::shader_program;
using namespace std::string_literals;

static GLuint get_shader_log(gl_shader &shader, std::string &log)
{
	GLint result, length;
	glGetShaderiv(shader.id(), GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader.id(), GL_INFO_LOG_LENGTH, &length);

	if (length > 0)
	{
		auto buf = std::make_unique<char[]>(length + 1);
		glGetShaderInfoLog(shader.id(), length, nullptr, buf.get());
		log = std::string(buf.get());
	}
	else
		log.clear();

	return result;
}

static GLuint get_program_log(gl_program &prog, std::string &log)
{
	GLint result, length;
	glGetProgramiv(prog.id(), GL_LINK_STATUS, &result);
	glGetProgramiv(prog.id(), GL_INFO_LOG_LENGTH, &length);

	if (length > 0)
	{
		auto buf = std::make_unique<char[]>(length + 1);
		glGetProgramInfoLog(prog.id(), length, nullptr, buf.get());
		log = std::string(buf.get());
	}
	else
		log.clear();

	return result;
}

gl_shader br::make_shader(GLenum type, const std::string &source)
{
	gl_shader shader(type);

	auto buf = std::make_unique<char[]>(source.length() + 1);
	auto ptr = buf.get();
	std::strncpy(buf.get(), source.c_str(), source.length() + 1);
	glShaderSource(shader.id(), 1, &ptr, nullptr);
	glCompileShader(shader.id());

	std::string log;
	if (get_shader_log(shader, log) == GL_FALSE)
		throw std::runtime_error("Shader compilation failed:\n"s + log + "\n"s);

	return shader;
}

shader_program::shader_program(std::initializer_list<const gl_shader*> shaders)
{
	for (const auto &sh : shaders)
		glAttachShader(id(), sh->id());

	glLinkProgram(id());

	std::string log;
	if (get_program_log(*this, log) == GL_FALSE)
		throw std::runtime_error("Program linking failed:\n"s + log + "\n"s);

	// Get uniforms
	GLint count;
	glGetProgramiv(id(), GL_ACTIVE_UNIFORMS, &count);

	for (int loc = 0; loc < count; loc++)
	{
		char buf[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(id(), loc, sizeof(buf), &length, &size, &type, buf);
		m_uniforms[buf] = loc;
	}
}

GLint shader_program::get_uniform_location(const std::string &name)
{
	auto it = m_uniforms.find(name);
	return it == m_uniforms.end() ? -1 : it->second;
}