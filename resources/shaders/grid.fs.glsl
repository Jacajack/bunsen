#version 420 core

/*
	Strongly inspired by http://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/
*/

in struct VS_OUT
{
	vec3 v_near;
	vec3 v_far;
} vs_out;

uniform vec3 cam_pos;
uniform vec3 cam_dir;
uniform float cam_near;
uniform float cam_far;
uniform mat4 mat_view;
uniform mat4 mat_proj;

out vec4 f_color;

vec4 grid(vec2 p)
{
	vec2 dp = fwidth(p);
	vec2 grid = abs(fract(p - 0.5) - 0.5) / dp;
	float line = min(grid.x, grid.y);
	float opacity = 1.0 - min(line, 1.0);
	vec3 color = vec3(0.3, 0.3, 0.3);
	float minx = min(dp.x, 1);
	float minz = min(dp.y, 1);

	if (abs(p.x) < 0.5 * minx)
		color.b = 1;

	if (abs(p.y) < 0.5 * minz)
		color.r = 1;

	return vec4(color, opacity);
}

float get_depth(vec3 pos)
{
	vec4 clip = mat_proj * mat_view * vec4(pos, 1);
	return 0.5 * (clip.z / clip.w + 1);
}

float get_linear_depth(vec3 pos)
{
	vec4 clip = mat_proj * mat_view * vec4(pos, 1);
	float clip_depth = clip.z / clip.w * 2 - 1;
	float lin_depth = 2 * cam_near * cam_far / (cam_far + cam_near - clip_depth * (cam_far - cam_near));
	return lin_depth / cam_far;
}

void main()
{
	vec3 near = vs_out.v_near;
	vec3 far = vs_out.v_far;
	float t = -near.y / (far.y - near.y);
	vec3 pos = near + t * (far - near);

	// float opacity = 1 - min(length(fwidth(pos.xz)), 1); // works but is quite sharp
	// float opacity = max(0, 0.8 - 8 * get_linear_depth(pos));
	float angle = abs(atan(cam_pos.y, length(cam_pos.xz - pos.xz)));
	float dist = length(pos - cam_pos);
	float atten = dot(normalize(pos - cam_pos), cam_dir);
	float opacity = atten * smoothstep(0, radians(20), angle) * smoothstep(40, 25, dist);
	vec4 color = grid(pos.xz);
	f_color = vec4(color.rgb, color.a * (t > 0 ? opacity : 0));
	gl_FragDepth = get_depth(pos);
}