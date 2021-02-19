#version 420 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_normal;

uniform mat4 mat_model;
uniform mat4 mat_view;
uniform mat4 mat_proj;

void main()
{
	// Vertex in camera space
	vec4 cv = mat_view * mat_model * vec4(v_pos, 1.0); 
	vec4 v = mat_proj * cv;
	v /= v.w;
	
	// Pass on projected vertex and z value in camera space
	gl_Position =  vec4(v.xy, cv.z, 1);
}
