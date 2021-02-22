#include "shader.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include "gl.hpp"
#include "../log.hpp"
#include "../bunsen.hpp"

using bu::gl_shader;
using bu::gl_program;
using bu::shader_program;
using namespace std::string_literals;
namespace fs = std::filesystem;

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

gl_shader bu::make_shader(GLenum type, const std::string &source)
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
	if (log.size())
		LOG_WARNING << "Shader compilation log: " << log;

	return shader;
}

shader_program::shader_program(std::initializer_list<const gl_shader*> shaders) :
	shader_program(std::vector<const gl_shader*>(shaders.begin(), shaders.end()))
{
}

shader_program::shader_program(std::vector<const gl_shader*> shaders)
{
	for (const auto &sh : shaders)
		glAttachShader(id(), sh->id());

	glLinkProgram(id());

	std::string log;
	if (get_program_log(*this, log) == GL_FALSE)
		throw std::runtime_error("Program linking failed:\n"s + log + "\n"s);
	if (log.size())
		LOG_WARNING << "Shader linking log: " << log;

	// Get uniforms
	GLint count;
	glGetProgramiv(id(), GL_ACTIVE_UNIFORMS, &count);

	for (int i = 0; i < count; i++)
	{
		char buf[256];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(id(), i, sizeof(buf), &length, &size, &type, buf);
		auto loc = glGetUniformLocation(id(), buf);
		m_uniforms[buf] = loc;
	}
}

GLint shader_program::get_uniform_location(const std::string &name)
{
	auto it = m_uniforms.find(name);
	if (it == m_uniforms.end())
	{
		// LOG_DEBUG << "Requested nonexistent uniform '" << name << "'";
		return -1;
	}
	return it->second;
}

/**
	\brief Loads entire shader pipeline from shader directory specified in the config
*/
shader_program bu::load_shader_program(const std::string &name)
{
	const static std::map<std::string, GLenum> shader_extensions{
		{".vs", GL_VERTEX_SHADER},
		{".fs", GL_FRAGMENT_SHADER},
		{".gs", GL_GEOMETRY_SHADER},
		{".cs", GL_COMPUTE_SHADER},
		{".tcs", GL_TESS_CONTROL_SHADER},
		{".tes", GL_TESS_EVALUATION_SHADER},
	};

	auto dir = bu::bunsen::get().config.general.shader_dir;

	LOG_INFO << "Loading shader '" << name << "'..."; 

	// Get all shader filenames
	std::vector<fs::path> files;
	for (const auto &entry : fs::directory_iterator(dir))
		if (entry.path().stem().stem() == name && entry.path().extension() == ".glsl")
			files.push_back(entry.path());

	// Shader files not found
	if (files.empty())
	{
		LOG_ERROR << "Could not find shader named '" << name << "'!";
		throw std::runtime_error("Could not load shader '"s + name + "'"s);
	}

	std::vector<bu::gl_shader> shaders;
	for (auto &f : files)
	{
		auto shader_ext = f.stem().extension().string();
		auto it = shader_extensions.find(shader_ext);
		if (it != shader_extensions.end())
		{
			// LOG_DEBUG << "Loading shader file " << f.string();
			shaders.push_back(bu::make_shader(it->second, bu::slurp_txt(f.string())));
		}
	}

	// No valid shader files
	if (shaders.empty())
	{
		LOG_ERROR << "Could not find any valid shader files for '" << name << "'!";
		throw std::runtime_error("Could not find any valid shader files for '"s + name + "'"s);
	}

	std::vector<const bu::gl_shader*> shader_ptrs;
	for (auto &s : shaders)
		shader_ptrs.push_back(&s);

	return shader_program(shader_ptrs);
}
