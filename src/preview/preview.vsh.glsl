#version 430 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_normal;

uniform mat4 mat_model;
uniform mat4 mat_view;
uniform mat4 mat_proj;

out struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
} vs_out;

void main()
{
	vs_out.v_pos = v_pos;
	vs_out.v_normal = v_normal;
	gl_Position = mat_proj * mat_view * mat_model * vec4(v_pos, 1.0);
}