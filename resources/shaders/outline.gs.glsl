#version 420 core

layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 6) out;

uniform float aspect;

void emit(vec2 v)
{
	gl_Position = vec4(v.xy, 0, 1);
	EmitVertex();
}

void main()
{
	const vec2 offset = vec2(0.003) * vec2(1, aspect);

	if (gl_in[0].gl_Position.z > 0 || gl_in[1].gl_Position.z > 0 || gl_in[2].gl_Position.z > 0)
		return;
	
	vec2 v0 = gl_in[0].gl_Position.xy;
	vec2 v1 = gl_in[1].gl_Position.xy;
	vec2 v2 = gl_in[2].gl_Position.xy;

	vec2 E0 = v1 - v0;
	vec2 E1 = v2 - v1;
	
	// Flip back-facing triangles so that one-sided shapes have outlines too
	if (E0.x * E1.y - E0.y * E1.x < 0)
	{
		vec2 tmp = v0;
		v0 = v1;
		v1 = tmp;
		E0 = v1 - v0;
		E1 = v2 - v1;
	}

	vec2 E2 = v0 - v2;

	vec2 n0 = normalize(E0 * vec2(-1, 1)).yx;
	vec2 n1 = normalize(E1 * vec2(-1, 1)).yx;
	vec2 n2 = normalize(E2 * vec2(-1, 1)).yx;

	vec2 v00 = v0 + offset * n0;
	vec2 v02 = v0 + offset * n2;
	vec2 v10 = v1 + offset * n0;
	vec2 v11 = v1 + offset * n1;
	vec2 v21 = v2 + offset * n1;
	vec2 v22 = v2 + offset * n2;

	// vec2 e0 = normalize(E0);
	// vec2 e1 = normalize(E1);
	// vec2 e2 = normalize(E2);

	// vec2 v00 = v0 + offset * e2;
	// vec2 v02 = v0 - offset * e1;
	// vec2 v10 = v1 - offset * e1;
	// vec2 v11 = v1 + offset * e0;
	// vec2 v21 = v2 - offset * e2;
	// vec2 v22 = v2 + offset * e1;

	emit(v02);
	emit(v00);
	emit(v22);
	emit(v10);
	emit(v21);
	emit(v11);
	EndPrimitive();
}
