#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedoSpec;
layout(location = 2) out vec4 gMetallicRoughness;

in VS_OUT
{
	vec3 Normal;
} vs_out;

void main()
{
	vec4 color = vec4(1.0, 1.0, 0.0, 1.0);
	gAlbedoSpec = color;

	gNormal = vec4(vs_out.Normal / 2.0 + 0.5, 1.0);

	gMetallicRoughness = vec4(0.f, 1.f, 0.f, 0.f);
}