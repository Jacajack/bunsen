#version 420 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_normal;

uniform mat4 mat_model;
uniform mat4 mat_view;
uniform mat4 mat_proj;

void main()
{
	mat4 mat_mvp = mat_proj * mat_view * mat_model;  
	vec4 v = mat_mvp * vec4(v_pos * 1.0, 1.0);
	v /= v.w;
	gl_Position =  vec4(v.xy, 0, 1);
}