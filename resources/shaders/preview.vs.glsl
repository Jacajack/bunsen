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
	mat4 mat_mv = mat_view * mat_model;
	vec4 v_pos_view = mat_mv * vec4(v_pos, 1.0);

	/*
		Compute overall scaling factor of the transform on each axis.
		Dividing by scale squared is equivalent to computing normalized
		rotation matrix first and then applying 1/s scaling to it.
	*/
	mat3 mat_mvn = mat3(mat_mv);
	mat_mvn[0] /= dot(mat_mvn[0], mat_mvn[0]);
	mat_mvn[1] /= dot(mat_mvn[1], mat_mvn[1]);
	mat_mvn[2] /= dot(mat_mvn[2], mat_mvn[2]);
	vec3 v_normal_view = normalize(mat_mvn * v_normal);

	vs_out.v_pos = v_pos_view.xyz;
	vs_out.v_normal = v_normal_view;
	
	gl_Position = mat_proj * v_pos_view;
}