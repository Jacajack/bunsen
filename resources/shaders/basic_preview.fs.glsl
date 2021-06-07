#version 420 core

in struct VS_OUT
{
	vec3 v_pos;    // Vertex position in camera space
	vec3 v_normal; // Vertex normal in camera space
} vs_out;

uniform vec3 world_color;
uniform vec3 base_color;
uniform float specular_int;

out vec4 f_color;

void main()
{
	const vec3 light_pos = vec3(-2, 1, 1); // In camera space
	const vec3 light_color = vec3(1);
	const float ambient_int = 1.0;
	const float diffuse_int = 1.0;
	const float specular_exp = 10.0;

	vec3 light_int = light_color;

	vec3 N = normalize(vs_out.v_normal);
	vec3 L = normalize(vec3(-0.2, 0.2, 1));
	vec3 V = -normalize(vs_out.v_pos);

	// Flip normals if necessary
	if (dot(N, V) < 0)
		N = -N;

	vec3 H = normalize(L + V);

	vec3 ambient = base_color * world_color;
	vec3 diffuse = light_int * base_color * clamp(dot(N, L), 0, 1);
	vec3 specular = light_int * vec3(pow(clamp(dot(N, H), 0, 1), specular_exp));

	vec3 color = ambient * ambient_int + diffuse * diffuse_int + specular * specular_int;

	// Gamma correction
	color = pow(color, vec3(1 / 2.2));
	f_color = vec4(color, 1);
}