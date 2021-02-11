#include "gl.hpp"
#include "../log.hpp"
#include <unordered_map>

void br::gl_debug_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *user)
{
	using namespace std::string_literals;

	static const std::unordered_map<GLenum, std::string> source_names
	{
		{GL_DEBUG_SOURCE_API, "API"},
		{GL_DEBUG_SOURCE_SHADER_COMPILER, "shader compiler"},
		{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "window system"},
		{GL_DEBUG_SOURCE_THIRD_PARTY, "third party"},
		{GL_DEBUG_SOURCE_APPLICATION, "application"},
		{GL_DEBUG_SOURCE_OTHER, "other"},
	};

	static const std::unordered_map<GLenum, std::string> type_names
	{
		{GL_DEBUG_TYPE_ERROR, "error"},
		{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "deprecated behavior"},
		{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "undefined behavior"},
		{GL_DEBUG_TYPE_PERFORMANCE, "performance issue"},
		{GL_DEBUG_TYPE_PORTABILITY, "portability issue"},
		{GL_DEBUG_TYPE_MARKER, "marker"},
		{GL_DEBUG_TYPE_PUSH_GROUP, "group push"},
		{GL_DEBUG_TYPE_POP_GROUP, "group pop"},
		{GL_DEBUG_TYPE_OTHER, "other"},
	};

	static const std::unordered_map<GLenum, std::string> severity_names
	{
		{GL_DEBUG_SEVERITY_LOW, "low"},
		{GL_DEBUG_SEVERITY_MEDIUM, "med"},
		{GL_DEBUG_SEVERITY_HIGH, "high"},
		{GL_DEBUG_SEVERITY_NOTIFICATION, "info"},
	};

	try
	{
		br::log_stream(
			std::cerr, 
			"[GL] "s + type_names.at(type) + ": "s, 
			type == GL_DEBUG_TYPE_ERROR ? 1 : 6
		) << message;
	}
	catch (...)
	{
		LOG_ERROR << "Failed to print proper GL error message!";
	}
}