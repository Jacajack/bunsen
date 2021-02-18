#version 420 core
#define M_PI 3.14159265358979323846

uniform vec3 color;

in struct VS_OUT
{
	vec2 v_pos;
	float depth;
} vs_out;

out vec4 f_color;

void main()
{
	vec2 pos = vs_out.v_pos;
	float z = length(pos) * 4;
	float arg = atan(pos.y, pos.x);
	float narg = arg / 2.0 / M_PI;

	float c = 1;
	c *= smoothstep(0.02, 0.04, z - 0.2);
	c *= max(smoothstep(0.02, 0.04, abs(z - 0.4)), float(fract(narg * 10) < 0.5));
	c *= max(smoothstep(0.02, 0.04, abs(z - 0.6)), float(fract(narg * 5) < 0.5));
	
	if (c >= 1) discard;
	f_color = vec4(color, 1 - c);
	gl_FragDepth = vs_out.depth;
}