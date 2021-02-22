#version 420 core

uniform int samples;
uniform sampler2DMS tex;
uniform ivec2 size;

in struct VS_OUT
{
	vec2 v_pos;
} vs_out;

out vec4 f_color;

void main()
{
	vec2 uv = vs_out.v_pos * 0.5 + 0.5;
	ivec2 pos = ivec2(uv * size);

	vec4 color = vec4(0);
	for (int i = 0; i < samples; i++)
		color += texelFetch(tex, pos, i);
	
	f_color = color / samples;
}