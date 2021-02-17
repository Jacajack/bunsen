#version 420 core

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
	vec4 v_pos_view = mat_view * mat_model * vec4(v_pos, 1.0);
	vec4 v_normal_view = mat_view * mat_model * vec4(v_normal, 0.0);

	vs_out.v_pos = v_pos_view.xyz;
	vs_out.v_normal = v_normal_view.xyz;
	
	gl_Position = mat_proj * v_pos_view;
}