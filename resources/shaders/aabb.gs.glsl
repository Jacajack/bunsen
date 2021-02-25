#version 420 core

layout (lines) in;
layout (line_strip) out;
layout (max_vertices = 16) out;

uniform mat4 mat_view;
uniform mat4 mat_proj;

void emit(vec3 v)
{
	gl_Position = mat_proj * mat_view * vec4(v, 1);
	EmitVertex();
}

void main()
{
	vec3 v000 = gl_in[0].gl_Position.xyz;
	vec3 v111 = gl_in[1].gl_Position.xyz;
	vec3 v001 = vec3(v000.xy, v111.z);
	vec3 v011 = vec3(v000.x, v111.yz);
	vec3 v110 = vec3(v111.xy, v000.z);
	vec3 v100 = vec3(v111.x, v000.yz);
	vec3 v010 = vec3(v000.x, v111.y, v000.z);
	vec3 v101 = vec3(v111.x, v000.y, v111.z);

	emit(v000);
	emit(v100);
	emit(v101);
	emit(v100);
	emit(v110);
	emit(v111);
	emit(v110);
	emit(v010);
	emit(v011);
	emit(v010);
	emit(v000);
	emit(v001);
	emit(v101);
	emit(v111);
	emit(v011);
	emit(v001);

	EndPrimitive();
}
