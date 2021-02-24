#version 420 core

out struct VS_OUT
{
	vec2 v_pos;
} vs_out;

void main()
{
	const vec2 quad[6] = vec2[](
		vec2(-1, -1), vec2(1, -1), vec2(-1, 1),
		vec2(1, -1), vec2(1, 1), vec2(-1, 1)
	);

	vec2 v = quad[gl_VertexID];
	vs_out.v_pos = v;
	gl_Position = vec4(v, 0, 1);
}