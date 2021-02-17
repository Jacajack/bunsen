#version 420 core

uniform mat4 mat_view;
uniform mat4 mat_proj;

out struct VS_OUT
{
	vec3 v_near;
	vec3 v_far;
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
	const vec3 quad[6] = vec3[](
		vec3(-1, -1, 0), vec3(1, -1, 0), vec3(-1, 1, 0),
		vec3(1, -1, 0), vec3(1, 1, 0), vec3(-1, 1, 0)
	);

	vec3 v = quad[gl_VertexID];
	vs_out.v_near = unproject(vec3(v.xy, 0));
	vs_out.v_far = unproject(vec3(v.xy, 1));
	gl_Position = vec4(v, 1);
}