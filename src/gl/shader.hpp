#pragma once

#include "gl.hpp"
#include <string>
#include <map>
#include <vector>

namespace bu {

class shader_program : public gl_program
{
public:
	shader_program(std::initializer_list<const gl_shader*> shaders);
	shader_program(std::vector<const gl_shader*> shaders);
	GLint get_uniform_location(const std::string &name);

private:
	std::map<std::string, GLint> m_uniforms;
};

gl_shader make_shader(GLenum type, const std::string &source);
shader_program load_shader_program(const std::string &name);

}