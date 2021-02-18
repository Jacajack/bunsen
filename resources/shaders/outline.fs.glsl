#version 420 core

out vec4 f_color;

void main()
{
	f_color = vec4(0.8, 0.4, 0, 1);
	gl_FragDepth = 0;
}