#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 gAlbedoSpec;

in VS_OUT
{
	vec2 TexCoords;
	vec3 Normal;
} ps_in;

layout(std140, binding = 1) uniform LightBuffer
{
	vec3 Color;
} lBuffer;


void main()
{
	gNormal = vec4(ps_in.Normal / 2.0 + 0.5, 1.0);

	vec4 color = vec4(lBuffer.Color, 1.0);
	gAlbedoSpec = vec4(color.xyz, 1.0);
}