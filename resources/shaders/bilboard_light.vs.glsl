#version 420 core

uniform mat4 mat_view;
uniform mat4 mat_proj;
uniform vec3 position;
uniform vec2 size;

out struct VS_OUT
{
	vec2 v_pos;
	float depth;
} vs_out;

void main()
{
	const vec2 quad[6] = vec2[](
		vec2(-1, -1), vec2(1, -1), vec2(-1, 1),
		vec2(1, -1), vec2(1, 1), vec2(-1, 1)
	);

	// Compute depth
	vec4 center = mat_proj * mat_view * vec4(position, 1);
	float w = center.w;
	center /= w;
	vs_out.depth = center.z * 0.5 + 0.5;

	// Draw quad around projected center point
	vec4 v = mat_proj * mat_view * vec4(position, 1);
	v /= v.w;
	v.xy += quad[gl_VertexID] * size;
	vs_out.v_pos = quad[gl_VertexID];
	gl_Position = v;
}