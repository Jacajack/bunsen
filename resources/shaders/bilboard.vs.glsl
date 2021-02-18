#version 420 core

uniform mat4 mat_view;
uniform mat4 mat_proj;
uniform vec3 position;
uniform float size;

out struct VS_OUT
{
	vec2 v_pos;
	float depth;
} vs_out;

vec3 unproject(vec3 v)
{
	mat4 inv_proj = inverse(mat_proj);
	mat4 inv_view = inverse(mat_view);
	vec4 unpv = inv_view * inv_proj * vec4(v, 1);
	return (unpv / unpv.w).xyz;
}

void main()
{
	const vec2 quad[6] = vec2[](
		vec2(-1, -1), vec2(1, -1), vec2(-1, 1),
		vec2(1, -1), vec2(1, 1), vec2(-1, 1)
	);

	// Compute depth
	vec4 center = mat_proj * mat_view * vec4(position, 1);
	center /= center.w;
	vs_out.depth = center.z * 0.5 + 0.5;

	vec4 v = mat_view * vec4(position, 1);
	v.xy += quad[gl_VertexID] * size;
	vs_out.v_pos = quad[gl_VertexID];
	gl_Position = mat_proj * v;
}