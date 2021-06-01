#version 420 core

in struct VS_OUT
{
	vec2 v_pos;
} vs_out;

uniform sampler2D tex;
uniform ivec2 size;

out vec4 f_color;

void main()
{
	vec2 uv = vs_out.v_pos * 0.5 + 0.5;
	vec4 sampled = texelFetch(tex, ivec2(uv * size), 0);

	// If no samples, show background
	if (sampled.a == 0)
	{
		f_color = vec4(0);
		return;
	}

	vec3 color = max(vec3(0.0), sampled.rgb / sampled.a);
	
	// Reinhard
	color = color / (color + 1);

	// Gamma
	color = pow(color, vec3(1 / 2.2));
	f_color = vec4(color.rgb, 1);
}